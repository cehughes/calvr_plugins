#include "FuturePatient.h"
#include "DataGraph.h"

#include <cvrKernel/PluginHelper.h>
#include <cvrKernel/ComController.h>
#include <cvrConfig/ConfigManager.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#include <mysql++/mysql++.h>

using namespace cvr;

CVRPLUGIN(FuturePatient)

FuturePatient::FuturePatient()
{
    _conn = NULL;
    _layoutObject = NULL;
    _multiObject = NULL;
}

FuturePatient::~FuturePatient()
{
}

bool FuturePatient::init()
{
    /*DataGraph * dg = new DataGraph();
    srand(0);
    osg::Vec3Array * points = new osg::Vec3Array();
    for(int i = 0; i < 20; i++)
    {
	osg::Vec3 point(i / 20.0,0,(rand() % 1000)/1000.0);
	points->push_back(point);
	//std::cerr << "Point x: " << point.x() << " y: " << point.y() << " z: " << point.z() << std::endl;
    }

    dg->addGraph("TestGraph", points, POINTS_WITH_LINES, "X - Axis", "Z - Axis", osg::Vec4(0,1.0,0,1.0));
    dg->setZDataRange("TestGraph",-100,100);

    struct tm timestart,timeend;
    timestart.tm_year = 2007 - 1900;
    timestart.tm_mon = 3;
    timestart.tm_mday = 12;
    timestart.tm_hour = 14;
    timestart.tm_min = 20;
    timestart.tm_sec = 1;

    timeend.tm_year = 2008 - 1900;
    timeend.tm_mon = 5;
    timeend.tm_mday = 1;
    timeend.tm_hour = 20;
    timeend.tm_min = 4;
    timeend.tm_sec = 58;

    time_t tstart, tend;
    tstart = mktime(&timestart);
    tend = mktime(&timeend);

    dg->setXDataRangeTimestamp("TestGraph",tstart,tend);

    timestart.tm_year = 2003 - 1900;
    timeend.tm_year = 2007 - 1900;
    tstart = mktime(&timestart);
    tend = mktime(&timeend);*/

    //dg->setXDisplayRangeTimestamp(tstart,tend);

    //dg->setXDisplayRange(0.25,0.8);
    //PluginHelper::getObjectsRoot()->addChild(dg->getGraphRoot());

    //makeGraph("SIga");
    
    _fpMenu = new SubMenu("FuturePatient");

    _presetMenu = new SubMenu("Presets");
    _fpMenu->addItem(_presetMenu);

    _inflammationButton = new MenuButton("Big 4");
    _inflammationButton->setCallback(this);
    _presetMenu->addItem(_inflammationButton);

    _loadAll = new MenuButton("All");
    _loadAll->setCallback(this);
    _presetMenu->addItem(_loadAll);

    _groupLoadMenu = new SubMenu("Group Load");
    _fpMenu->addItem(_groupLoadMenu);

    _testList = new MenuList();
    _testList->setCallback(this);
    _fpMenu->addItem(_testList);

    _loadButton = new MenuButton("Load");
    _loadButton->setCallback(this);
    _fpMenu->addItem(_loadButton);

    _removeAllButton = new MenuButton("Remove All");
    _removeAllButton->setCallback(this);
    _fpMenu->addItem(_removeAllButton);

    _multiAddCB = new MenuCheckbox("Multi Add", false);
    _multiAddCB->setCallback(this);
    _fpMenu->addItem(_multiAddCB);

    PluginHelper::addRootMenuItem(_fpMenu);

    struct listField
    {
	char entry[256];
    };

    struct listField * lfList = NULL;
    int listEntries = 0;

    if(ComController::instance()->isMaster())
    {
	if(!_conn)
	{
	    _conn = new mysqlpp::Connection(false);
	    if(!_conn->connect("futurepatient","palmsdev2.ucsd.edu","fpuser","FPp@ssw0rd"))
	    {
		std::cerr << "Unable to connect to database." << std::endl;
		delete _conn;
		_conn = NULL;
	    }
	}

	if(_conn)
	{
	    mysqlpp::Query q = _conn->query("select distinct name from Measure order by name;");
	    mysqlpp::StoreQueryResult res = q.store();

	    listEntries = res.num_rows();

	    if(listEntries)
	    {
		lfList = new struct listField[listEntries];

		for(int i = 0; i < listEntries; i++)
		{
		    strncpy(lfList[i].entry,res[i]["name"].c_str(),255);
		}
	    }
	}

	ComController::instance()->sendSlaves(&listEntries,sizeof(int));
	if(listEntries)
	{
	    ComController::instance()->sendSlaves(lfList,sizeof(struct listField)*listEntries);
	}
    }
    else
    {
	ComController::instance()->readMaster(&listEntries,sizeof(int));
	if(listEntries)
	{
	    lfList = new struct listField[listEntries];
	    ComController::instance()->readMaster(lfList,sizeof(struct listField)*listEntries);
	}
    }

    std::vector<std::string> stringlist;
    for(int i = 0; i < listEntries; i++)
    {
	stringlist.push_back(lfList[i].entry);
    }

    _testList->setValues(stringlist);

    if(lfList)
    {
	delete[] lfList;
    }

    _groupList = new MenuList();
    _groupList->setCallback(this);
    _groupLoadMenu->addItem(_groupList);

    _groupLoadButton = new MenuButton("Load");
    _groupLoadButton->setCallback(this);
    _groupLoadMenu->addItem(_groupLoadButton);

    lfList = NULL;
    listEntries = 0;
    int * sizes = NULL;
    listField ** groupLists = NULL;

    if(ComController::instance()->isMaster())
    {
	if(_conn)
	{
	    mysqlpp::Query q = _conn->query("select display_name from Measure_Type order by display_name;");
	    mysqlpp::StoreQueryResult res = q.store();

	    listEntries = res.num_rows();

	    if(listEntries)
	    {
		lfList = new struct listField[listEntries];

		for(int i = 0; i < listEntries; i++)
		{
		    strncpy(lfList[i].entry,res[i]["display_name"].c_str(),255);
		}
	    }

	    if(listEntries)
	    {
		sizes = new int[listEntries];
		groupLists = new listField*[listEntries];

		for(int i = 0; i < listEntries; i++)
		{
		    std::stringstream groupss;
		    groupss << "select Measure.name from Measure inner join Measure_Type on Measure_Type.measure_type_id = Measure.measure_type_id where Measure_Type.display_name = \"" << res[i]["display_name"].c_str() << "\";";

		    mysqlpp::Query groupq = _conn->query(groupss.str().c_str());
		    mysqlpp::StoreQueryResult groupRes = groupq.store();

		    sizes[i] = groupRes.num_rows();
		    if(groupRes.num_rows())
		    {
			groupLists[i] = new listField[groupRes.num_rows()];
		    }
		    else
		    {
			groupLists[i] = NULL;
		    }

		    for(int j = 0; j < groupRes.num_rows(); j++)
		    {
			strncpy(groupLists[i][j].entry,groupRes[j]["name"].c_str(),255);
		    }
		}
	    }
	}

	ComController::instance()->sendSlaves(&listEntries,sizeof(int));
	if(listEntries)
	{
	    ComController::instance()->sendSlaves(lfList,sizeof(struct listField)*listEntries);
	    ComController::instance()->sendSlaves(sizes,sizeof(int)*listEntries);
	    for(int i = 0; i < listEntries; i++)
	    {
		if(sizes[i])
		{
		    ComController::instance()->sendSlaves(groupLists[i],sizeof(struct listField)*sizes[i]);
		}
	    }
	}
    }
    else
    {
	ComController::instance()->readMaster(&listEntries,sizeof(int));
	if(listEntries)
	{
	    lfList = new struct listField[listEntries];
	    ComController::instance()->readMaster(lfList,sizeof(struct listField)*listEntries);
	    sizes = new int[listEntries];
	    ComController::instance()->readMaster(sizes,sizeof(int)*listEntries);
	    groupLists = new listField*[listEntries];
	    for(int i = 0; i < listEntries; i++)
	    {
		if(sizes[i])
		{
		    groupLists[i] = new listField[sizes[i]];
		    ComController::instance()->readMaster(groupLists[i],sizeof(struct listField)*sizes[i]);
		}
		else
		{
		    groupLists[i] = NULL;
		}
	    }
	}
    }

    stringlist.clear();
    for(int i = 0; i < listEntries; i++)
    {
	stringlist.push_back(lfList[i].entry);

	_groupTestMap[lfList[i].entry] = std::vector<std::string>();
	for(int j = 0; j < sizes[i]; j++)
	{
	    _groupTestMap[lfList[i].entry].push_back(groupLists[i][j].entry);
	}
    }

    _groupList->setValues(stringlist);

    if(lfList)
    {
	delete[] lfList;
    }

    for(int i = 0; i < listEntries; i++)
    {
	if(groupLists[i])
	{
	    delete[] groupLists[i];
	}
    }

    if(listEntries)
    {
	delete[] sizes;
	delete[] groupLists;
    }

    /*if(_conn)
    {
	GraphObject * gobject = new GraphObject(_conn, 1000.0, 1000.0, "DataGraph", false, true, false, true, false);
	gobject->addGraph("LDL");
	PluginHelper::registerSceneObject(gobject,"FuturePatient");
	gobject->attachToScene();
    }*/

    return true;
}

void FuturePatient::menuCallback(MenuItem * item)
{
    if(item == _loadButton)
    {
	loadGraph(_testList->getValue());
	/*std::string value = _testList->getValue();
	if(!value.empty())
	{
	    if(_graphObjectMap.find(value) == _graphObjectMap.end())
	    {
		GraphObject * gobject = new GraphObject(_conn, 1000.0, 1000.0, "DataGraph", false, true, false, true, false);
		if(gobject->addGraph(value))
		{
		    _graphObjectMap[value] = gobject;
		}
		else
		{
		    delete gobject;
		}
	    }

	    if(_graphObjectMap.find(value) != _graphObjectMap.end())
	    {
		if(!_layoutObject)
		{
		    float width, height;
		    osg::Vec3 pos;
		    width = ConfigManager::getFloat("width","Plugin.FuturePatient.Layout",1500.0);
		    height = ConfigManager::getFloat("height","Plugin.FuturePatient.Layout",1000.0);
		    pos = ConfigManager::getVec3("Plugin.FuturePatient.Layout");
		    _layoutObject = new GraphLayoutObject(width,height,3,"GraphLayout",false,true,false,true,false);
		    _layoutObject->setPosition(pos);
		    PluginHelper::registerSceneObject(_layoutObject,"FuturePatient");
		    _layoutObject->attachToScene();
		}

		_layoutObject->addGraphObject(_graphObjectMap[value]);
	    }
	}*/
    }

    if(item == _groupLoadButton)
    {
	for(int i = 0; i < _groupTestMap[_groupList->getValue()].size(); i++)
	{
	    loadGraph(_groupTestMap[_groupList->getValue()][i]);
	}
    }

    if(item == _inflammationButton)
    {
	loadGraph("hs-CRP");
	loadGraph("SIgA");
	loadGraph("Lysozyme");
	loadGraph("Lactoferrin");
    }

    if(item == _multiAddCB)
    {
	if(_multiObject)
	{
	    if(!_multiObject->getNumGraphs())
	    {
		delete _multiObject;
	    }
	    _multiObject = NULL;
	}
    }

    if(item == _loadAll)
    {
	for(int i = 0; i < _testList->getListSize(); i++)
	{
	    loadGraph(_testList->getValue(i));
	}
    }

    if(item == _removeAllButton)
    {
	if(_layoutObject)
	{
	    _layoutObject->removeAll();
	}
	menuCallback(_multiAddCB);
    }
}

void FuturePatient::loadGraph(std::string name)
{
    if(!_layoutObject)
    {
	float width, height;
	osg::Vec3 pos;
	width = ConfigManager::getFloat("width","Plugin.FuturePatient.Layout",1500.0);
	height = ConfigManager::getFloat("height","Plugin.FuturePatient.Layout",1000.0);
	pos = ConfigManager::getVec3("Plugin.FuturePatient.Layout");
	_layoutObject = new GraphLayoutObject(width,height,3,"GraphLayout",false,true,false,true,false);
	_layoutObject->setPosition(pos);
	PluginHelper::registerSceneObject(_layoutObject,"FuturePatient");
	_layoutObject->attachToScene();
    }

    std::string value = name;
    if(!value.empty())
    {
	if(!_multiAddCB->getValue())
	{
	    if(_graphObjectMap.find(value) == _graphObjectMap.end())
	    {
		GraphObject * gobject = new GraphObject(_conn, 1000.0, 1000.0, "DataGraph", false, true, false, true, false);
		if(gobject->addGraph(value))
		{
		    _graphObjectMap[value] = gobject;
		}
		else
		{
		    delete gobject;
		}
	    }

	    if(_graphObjectMap.find(value) != _graphObjectMap.end())
	    {
		_layoutObject->addGraphObject(_graphObjectMap[value]);
	    }
	}
	else
	{
	    if(!_multiObject)
	    {
		_multiObject = new GraphObject(_conn, 1000.0, 1000.0, "DataGraph", false, true, false, true, false);
	    }

	    if(_multiObject->addGraph(value))
	    {
		if(_multiObject->getNumGraphs() == 1)
		{
		    _multiObject->setLayoutDoesDelete(true);
		    _layoutObject->addGraphObject(_multiObject);
		}
	    }
	}
    }
}

/*void FuturePatient::makeGraph(std::string name)
{
    mysqlpp::Connection conn(false);
    if(!conn.connect("futurepatient","palmsdev2.ucsd.edu","fpuser","FPp@ssw0rd"))
    {
	std::cerr << "Unable to connect to database." << std::endl;
	return;
    }

    std::stringstream qss;
    qss << "select Measurement.timestamp, unix_timestamp(Measurement.timestamp) as utime, Measurement.value from Measurement  inner join Measure  on Measurement.measure_id = Measure.measure_id  where Measure.name = \"" << name << "\" order by utime;";

    mysqlpp::Query query = conn.query(qss.str().c_str());
    mysqlpp::StoreQueryResult res;
    res = query.store();
    //std::cerr << "Num Rows: " << res.num_rows() << std::endl;
    if(!res.num_rows())
    {
	std::cerr << "Empty query result." << std::endl;
	return;
    }

    std::stringstream mss;
    mss << "select * from Measure where name = \"" << name << "\";";

    mysqlpp::Query metaQuery = conn.query(mss.str().c_str());
    mysqlpp::StoreQueryResult metaRes = metaQuery.store();

    if(!metaRes.num_rows())
    {
	std::cerr << "Meta Data query result empty." << std::endl;
	return;
    }

    osg::Vec3Array * points = new osg::Vec3Array(res.num_rows());
    osg::Vec4Array * colors = new osg::Vec4Array(res.num_rows());

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

    for(int i = 0; i < res.num_rows(); i++)
    {
	time_t time = atol(res[i]["utime"].c_str());
	float value = atof(res[i]["value"].c_str());
	points->at(i) = osg::Vec3((time-mint) / (double)(maxt-mint),0,(value-minval) / (maxval-minval));
	if(hasGoodRange)
	{
	    if(value < goodLow || value > goodHigh)
	    {
		colors->at(i) = osg::Vec4(1.0,0,0,1.0);
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

    DataGraph * dg = new DataGraph();
    dg->addGraph(metaRes[0]["display_name"].c_str(), points, POINTS_WITH_LINES, "Time", metaRes[0]["units"].c_str(), osg::Vec4(0,1.0,0,1.0),colors);
    dg->setZDataRange(metaRes[0]["display_name"].c_str(),minval,maxval);
    dg->setXDataRangeTimestamp(metaRes[0]["display_name"].c_str(),mint,maxt);
    PluginHelper::getObjectsRoot()->addChild(dg->getGraphRoot());
}*/
