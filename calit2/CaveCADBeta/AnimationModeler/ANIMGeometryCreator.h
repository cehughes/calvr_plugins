/***************************************************************
* File Name: ANIMGeometryCreator.h
*
* Description:
*
***************************************************************/

#ifndef _ANIM_GEOMETRY_CREATOR_H_
#define _ANIM_GEOMETRY_CREATOR_H_


// C++
#include <iostream>
#include <list>

// Open scene graph
#include <osg/BlendFunc>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/Material>
#include <osg/MatrixTransform>
#include <osg/Point>
#include <osg/PointSprite>
#include <osg/PositionAttitudeTransform>
#include <osg/Shader>
#include <osg/ShapeDrawable>
#include <osg/StateAttribute>
#include <osg/StateSet>
#include <osg/Switch>
#include <osg/Texture2D>
#include <osg/Vec3>
#include <osgDB/ReadFile>

// Local includes
#include "AnimationModelerBase.h"


namespace CAVEAnimationModeler
{

    #define ANIM_GEOMETRY_CREATOR_SHAPE_FLIP_TIME	1.0f
    #define ANIM_GEOMETRY_CREATOR_SHAPE_FLIP_SAMPS	36


    /***************************************************************
    * Class: ANIMShapeSwitchEntry
    ***************************************************************/
    class ANIMShapeSwitchEntry
    {
      public:
        osg::Switch *mSwitch;
        osg::AnimationPathCallback *mFlipUpFwdAnim, *mFlipDownFwdAnim;
        osg::AnimationPathCallback *mFlipUpBwdAnim, *mFlipDownBwdAnim;
    };

    /* Functions called by 'DSGeometryCreator' */
    void ANIMLoadGeometryCreator(osg::PositionAttitudeTransform** xformScaleFwd, 
			  	 osg::PositionAttitudeTransform** xformScaleBwd,
				 osg::Switch **sphereExteriorSwitch, osg::Geode **sphereExteriorGeode,
				 int &numTypes, ANIMShapeSwitchEntry ***shapeSwitchEntryArray);
    void ANIMCreateSingleShapeSwitchAnimation(ANIMShapeSwitchEntry **shapeEntry, const CAVEGeodeShape::Type &typ);

    /* Function called by 'DOGeometryCreator' */
    void ANIMLoadGeometryCreatorReference(osg::Switch **snapWireframeSwitch, osg::Switch **snapSolidshapeSwitch);
};


#endif

