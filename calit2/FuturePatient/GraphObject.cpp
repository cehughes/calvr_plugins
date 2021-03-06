#include "GraphObject.h"

#include <cvrKernel/ComController.h>
#include <cvrConfig/ConfigManager.h>

#include <iostream>
#include <sstream>
#include <cstdlib>

using namespace cvr;

GraphObject::GraphObject(mysqlpp::Connection * conn, float width, float height, std::string name, bool navigation, bool movable, bool clip, bool contextMenu, bool showBounds) : TiledWallSceneObject(name,navigation,movable,clip,contextMenu,showBounds)
{
    _conn = conn;
    _graph = new DataGraph();
    _graph->setDisplaySize(width,height);

    setBoundsCalcMode(SceneObject::MANUAL);
    osg::BoundingBox bb(-(width*0.5),-2,-(height*0.5),width*0.5,0,height*0.5);
    setBoundingBox(bb);

    std::vector<std::string> mgdText;
    mgdText.push_back("Normal");
    mgdText.push_back("Color");
    mgdText.push_back("Shape");
    mgdText.push_back("Shape and Color");

    _mgdList = new MenuList();
    _mgdList->setValues(mgdText);
    _mgdList->setCallback(this);
    addMenuItem(_mgdList);

    _pdfDir = ConfigManager::getEntry("value","Plugin.FuturePatient.PDFDir","");

    _activeHand = -1;
    _layoutDoesDelete = false;
}

GraphObject::~GraphObject()
{

}

bool GraphObject::addGraph(std::string name)
{
    for(int i = 0; i < _nameList.size(); i++)
    {
	if(name == _nameList[i])
	{
	    return false;
	}
    }

    osg::ref_ptr<osg::Vec3Array> points;
    osg::ref_ptr<osg::Vec4Array> colors;

    struct graphData
    {
	char displayName[256];
	char units[256];
	float minValue;
	float maxValue;
	time_t minTime;
	time_t maxTime;
	int numPoints;
	int numAnnotations;
	bool valid;
    };

    struct pointAnnotation
    {
	int point;
	char text[1024];
	char url[2048];
    };

    struct graphData gd;
    gd.numAnnotations = 0;

    struct pointAnnotation * annotations = NULL;

    if(ComController::instance()->isMaster())
    {
	if(_conn)
	{
	    std::stringstream mss;
	    mss << "select * from Measure where name = \"" << name << "\";";

	    //std::cerr << "Query: " << mss.str() << std::endl;

	    mysqlpp::Query metaQuery = _conn->query(mss.str().c_str());
	    mysqlpp::StoreQueryResult metaRes = metaQuery.store();

	    if(!metaRes.num_rows())
	    {
		std::cerr << "Meta Data query result empty for value: " << name << std::endl;
		gd.valid = false;
	    }
	    else
	    {

		int measureId = atoi(metaRes[0]["measure_id"].c_str());

		std::stringstream qss;
		qss << "select Measurement.timestamp, unix_timestamp(Measurement.timestamp) as utime, Measurement.value, Measurement.has_annotation from Measurement inner join Measure on Measurement.measure_id = Measure.measure_id where Measure.measure_id = \"" << measureId << "\" order by utime;";

		//std::cerr << "Query: " << qss.str() << std::endl;

		mysqlpp::Query query = _conn->query(qss.str().c_str());
		mysqlpp::StoreQueryResult res;
		res = query.store();

		std::stringstream annotationss;
		annotationss << "select Annotation.text, Annotation.URL, unix_timestamp(Measurement.timestamp) as utime from Measurement inner join Annotation on Measurement.measurement_id = Annotation.measurement_id where Measurement.measure_id = \"" << measureId << "\" order by utime;";

		mysqlpp::Query annotationQuery = _conn->query(annotationss.str().c_str());
		mysqlpp::StoreQueryResult annotationRes = annotationQuery.store();

		//std::cerr << "Num Rows: " << res.num_rows() << std::endl;
		if(!res.num_rows())
		{
		    std::cerr << "Empty query result for name: " << name << " id: " << measureId << std::endl;
		    gd.valid = false;
		}
		else
		{

		    points = new osg::Vec3Array(res.num_rows());
		    colors = new osg::Vec4Array(res.num_rows());

		    bool hasGoodRange = false;
		    float goodLow, goodHigh;

		    if(strcmp(metaRes[0]["good_low"].c_str(),"NULL") && metaRes[0]["good_high"].c_str())
		    {
			hasGoodRange = true;
			goodLow = atof(metaRes[0]["good_low"].c_str());
			goodHigh = atof(metaRes[0]["good_high"].c_str());
		    }

		    //find min/max values
		    time_t mint, maxt;
		    mint = maxt = atol(res[0]["utime"].c_str());
		    float minval,maxval;
		    minval = maxval = atof(res[0]["value"].c_str());
		    for(int i = 1; i < res.num_rows(); i++)
		    {
			time_t time = atol(res[i]["utime"].c_str());
			float value = atof(res[i]["value"].c_str());

			if(time < mint)
			{
			    mint = time;
			}
			if(time > maxt)
			{
			    maxt = time;
			}
			if(value < minval)
			{
			    minval = value;
			}
			if(value > maxval)
			{
			    maxval = value;
			}
		    }

		    //std::cerr << "Mintime: " << mint << " Maxtime: " << maxt << " MinVal: " << minval << " Maxval: " << maxval << std::endl;

		    // remove range size of zero
		    if(minval == maxval)
		    {
			minval -= 1.0;
			maxval += 1.0;
		    }

		    if(mint == maxt)
		    {
			mint -= 86400;
			maxt += 86400;
		    }

		    int annCount = 0;
		    for(int i = 0; i < res.num_rows(); i++)
		    {
			time_t time = atol(res[i]["utime"].c_str());
			float value = atof(res[i]["value"].c_str());
			int hasAnn = atoi(res[i]["has_annotation"].c_str());
			if(hasAnn)
			{
			    annCount++;
			}

			points->at(i) = osg::Vec3((time-mint) / (double)(maxt-mint),0,(value-minval) / (maxval-minval));
			if(hasGoodRange)
			{
			    if(value < goodLow || value > goodHigh)
			    {
				if(value > goodHigh)
				{
				    float mult = value / goodHigh;
				    if(mult <= 10.0)
				    {
					colors->at(i) = osg::Vec4(1.0,0.5,0.25,1.0);
				    }
				    else if(mult <= 100.0)
				    {
					colors->at(i) = osg::Vec4(1.0,0,0,1.0);
				    }
				    else
				    {
					colors->at(i) = osg::Vec4(1.0,0,1.0,1.0);
				    }
				}
				else
				{
				    colors->at(i) = osg::Vec4(1.0,0,1.0,1.0);
				}
			    }
			    else
			    {
				colors->at(i) = osg::Vec4(0,1.0,0,1.0);
			    }
			}
			else
			{
			    colors->at(i) = osg::Vec4(0,0,1.0,1.0);
			}
		    }
		    gd.valid = true;

		    strncpy(gd.displayName, metaRes[0]["display_name"].c_str(), 255);
		    strncpy(gd.units, metaRes[0]["units"].c_str(), 255);
		    gd.minValue = minval;
		    gd.maxValue = maxval;
		    gd.minTime = mint;
		    gd.maxTime = maxt;
		    gd.numPoints = res.num_rows();

		    annCount = std::min(annCount,(int)annotationRes.num_rows());

		    if(annCount)
		    {
			annotations = new pointAnnotation[annCount];
		    }

		    int annIndex = 0;
		    for(int i = 0; i < res.num_rows(); i++)
		    {
			if(annIndex >= annotationRes.num_rows())
			{
			    break;
			}

			int hasAnn = atoi(res[i]["has_annotation"].c_str());
			if(hasAnn)
			{
			    annotations[annIndex].point = i;
			    strncpy(annotations[annIndex].text, annotationRes[annIndex]["text"].c_str(), 1023);
			    strncpy(annotations[annIndex].url, annotationRes[annIndex]["URL"].c_str(), 2047);
			    annIndex++;
			}
		    }
		    gd.numAnnotations = annCount;
		}
	    }
	}
	else
	{
	    std::cerr << "No database connection." << std::endl;
	    gd.valid = false;
	}

	ComController::instance()->sendSlaves(&gd,sizeof(struct graphData));
	if(gd.valid)
	{
	    ComController::instance()->sendSlaves((void*)points->getDataPointer(),points->size()*sizeof(osg::Vec3));
	    ComController::instance()->sendSlaves((void*)colors->getDataPointer(),colors->size()*sizeof(osg::Vec4));
	    if(gd.numAnnotations)
	    {
		ComController::instance()->sendSlaves((void*)annotations,sizeof(struct pointAnnotation)*gd.numAnnotations);
	    }
	}
    }
    else
    {
	ComController::instance()->readMaster(&gd,sizeof(struct graphData));
	if(gd.valid)
	{
	    osg::Vec3 * pointData = new osg::Vec3[gd.numPoints];
	    osg::Vec4 * colorData = new osg::Vec4[gd.numPoints];
	    ComController::instance()->readMaster(pointData,gd.numPoints*sizeof(osg::Vec3));
	    ComController::instance()->readMaster(colorData,gd.numPoints*sizeof(osg::Vec4));
	    points = new osg::Vec3Array(gd.numPoints,pointData);
	    colors = new osg::Vec4Array(gd.numPoints,colorData);

	    if(gd.numAnnotations)
	    {
		annotations = new struct pointAnnotation[gd.numAnnotations];
		ComController::instance()->readMaster(annotations,gd.numAnnotations*sizeof(struct pointAnnotation));
	    }
	}
    }

    if(gd.valid)
    {
	_graph->addGraph(gd.displayName, points, POINTS_WITH_LINES, "Time", gd.units, osg::Vec4(0,1.0,0,1.0),colors);
	_graph->setZDataRange(gd.displayName,gd.minValue,gd.maxValue);
	_graph->setXDataRangeTimestamp(gd.displayName,gd.minTime,gd.maxTime);
	addChild(_graph->getGraphRoot());
	_nameList.push_back(name);

	if(gd.numAnnotations)
	{
	    std::map<int,PointAction*> actionMap;

	    for(int i = 0; i < gd.numAnnotations; i++)
	    {
		//TODO: add directory path to urls
		actionMap[annotations[i].point] = new PointActionPDF(_pdfDir + "/" + annotations[i].url);
	    }

	    _graph->setPointActions(gd.displayName,actionMap);
	}
    }

    if(annotations)
    {
	delete[] annotations;
    }

    std::cerr << "Graph added with " << gd.numAnnotations << " annotations" << std::endl;

    return gd.valid;
}

void GraphObject::setGraphSize(float width, float height)
{
    _graph->setDisplaySize(width,height);

    osg::BoundingBox bb(-(width*0.5),-2,-(height*0.5),width*0.5,0,height*0.5);
    setBoundingBox(bb);
}

void GraphObject::setGraphDisplayRange(time_t start, time_t end)
{
    _graph->setXDisplayRangeTimestamp(start,end);
}

void GraphObject::resetGraphDisplayRange()
{
    time_t min, max;

    min = getMinTimestamp();
    max = getMaxTimestamp();

    if(min && max)
    {
	setGraphDisplayRange(min,max);
    }
}

void GraphObject::getGraphDisplayRange(time_t & start, time_t & end)
{
    _graph->getXDisplayRangeTimestamp(start,end);
}

time_t GraphObject::getMaxTimestamp()
{
    std::vector<std::string> names;
    _graph->getGraphNameList(names);

    time_t max;
    if(names.size())
    {
	max = _graph->getMaxTimestamp(names[0]);
    }
    else
    {
	max = 0;
    }

    for(int i = 1; i < names.size(); i++)
    {
	time_t temp = _graph->getMaxTimestamp(names[i]);
	if(temp > max)
	{
	    max = temp;
	}
    }

    return max;
}

time_t GraphObject::getMinTimestamp()
{
    std::vector<std::string> names;
    _graph->getGraphNameList(names);

    time_t min;
    if(names.size())
    {
	min = _graph->getMinTimestamp(names[0]);
    }
    else
    {
	min = 0;
    }

    for(int i = 1; i < names.size(); i++)
    {
	time_t temp = _graph->getMinTimestamp(names[i]);
	if(temp && temp < min)
	{
	    min = temp;
	}
    }

    return min;
}

void GraphObject::setBarPosition(float pos)
{
    _graph->setBarPosition(pos);
}

float GraphObject::getBarPosition()
{
    return _graph->getBarPosition();
}

void GraphObject::setBarVisible(bool b)
{
    _graph->setBarVisible(b);
}

bool GraphObject::getBarVisible()
{
    return _graph->getBarVisible();
}

bool GraphObject::getGraphSpacePoint(const osg::Matrix & mat, osg::Vec3 & point)
{
    osg::Matrix m;
    m = mat * getWorldToObjectMatrix();
    return _graph->getGraphSpacePoint(m,point);
}

void GraphObject::menuCallback(MenuItem * item)
{
    if(item == _mgdList)
    {
	//std::cerr << "Got index: " << _mgdList->getIndex() << std::endl;
	_graph->setMultiGraphDisplayMode((MultiGraphDisplayMode)_mgdList->getIndex());
	return;
    }

    TiledWallSceneObject::menuCallback(item);
}

bool GraphObject::processEvent(InteractionEvent * ie)
{
    TrackedButtonInteractionEvent * tie = ie->asTrackedButtonEvent();
    if(tie)
    {
	if(tie->getHand() == _activeHand && (tie->getInteraction() == cvr::BUTTON_DOWN || tie->getInteraction() == cvr::BUTTON_DOUBLE_CLICK))
	{
	    return _graph->pointClick();
	}
    }
    return TiledWallSceneObject::processEvent(ie);
}

void GraphObject::enterCallback(int handID, const osg::Matrix &mat)
{
}

void GraphObject::updateCallback(int handID, const osg::Matrix &mat)
{
    if(_activeHand >= 0 && handID != _activeHand)
    {
	return;
    }

    osg::Matrix m;
    m = mat * getWorldToObjectMatrix();

    if(_graph->displayHoverText(m))
    {
	_activeHand = handID;
    }
    else if(_activeHand >= 0)
    {
	_activeHand = -1;
    }
}

void GraphObject::leaveCallback(int handID)
{
    if(handID == _activeHand)
    {
	_activeHand = -1;
	_graph->clearHoverText();
    }
}
