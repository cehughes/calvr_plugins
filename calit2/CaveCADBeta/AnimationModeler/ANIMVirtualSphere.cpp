/***************************************************************
* Animation File Name: ANIMVirtualSphere.cpp
*
* Description: Create animated model of virtual sphere
*
* Written by ZHANG Lelin on Sep 15, 2010
*
***************************************************************/
#include "ANIMVirtualSphere.h"

using namespace std;
using namespace osg;

namespace CAVEAnimationModeler
{


/***************************************************************
* Function: ANIMCreateVirtualSphere()
*
***************************************************************/
void ANIMCreateVirtualSphere(osg::PositionAttitudeTransform** xformScaleFwd, 
			     osg::PositionAttitudeTransform** xformScaleBwd)
{
    // create sphere geometry
    *xformScaleFwd = new PositionAttitudeTransform;
    *xformScaleBwd = new PositionAttitudeTransform;
    Geode* sphereGeode = new Geode();
    Sphere* virtualSphere = new Sphere();
    Drawable* sphereDrawable = new ShapeDrawable(virtualSphere);

    virtualSphere->setRadius(ANIM_VIRTUAL_SPHERE_RADIUS);
    sphereGeode->addDrawable(sphereDrawable);
    (*xformScaleFwd)->addChild(sphereGeode);
    (*xformScaleBwd)->addChild(sphereGeode);

    osg::StateSet* stateset;   

    // highlights
/*    Sphere* highlightSphere = new Sphere();
    ShapeDrawable* highlightDrawable = new ShapeDrawable(highlightSphere);
    Geode* highlightGeode = new Geode();
    highlightSphere->setRadius(ANIM_VIRTUAL_SPHERE_RADIUS * 1.3);
    highlightDrawable->setColor(osg::Vec4(0,0,1,0.3));
    highlightGeode->addDrawable(highlightDrawable);
    (*xformScaleFwd)->addChild(highlightGeode);
    (*xformScaleBwd)->addChild(highlightGeode);

    stateset = highlightDrawable->getOrCreateStateSet();
    stateset->setMode(GL_BLEND, StateAttribute::ON);
    stateset->setMode(GL_CULL_FACE, StateAttribute::ON);
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateset->setRenderingHint(StateSet::TRANSPARENT_BIN);
*/

    // set up the forward / backward scale animation path
    AnimationPath* animationPathScaleFwd = new AnimationPath;
    AnimationPath* animationPathScaleBwd = new AnimationPath;
    animationPathScaleFwd->setLoopMode(AnimationPath::NO_LOOPING);
    animationPathScaleBwd->setLoopMode(AnimationPath::NO_LOOPING);

    osg::Vec3 pos(-1.5, 0, 0);

    Vec3 scaleFwd, scaleBwd;
    float step = 1.f / ANIM_VIRTUAL_SPHERE_NUM_SAMPS;
    for (int i = 0; i < ANIM_VIRTUAL_SPHERE_NUM_SAMPS + 1; i++)
    {
        float val = i * step;
        scaleFwd = Vec3(val, val, val);
        scaleBwd = Vec3(1-val, 1-val, 1-val);
        animationPathScaleFwd->insert(val, AnimationPath::ControlPoint(pos, Quat(), scaleFwd));
        animationPathScaleBwd->insert(val, AnimationPath::ControlPoint(pos, Quat(), scaleBwd));
    }

    AnimationPathCallback *animCallbackFwd = new AnimationPathCallback(animationPathScaleFwd,
						0.0, 1.f / ANIM_VIRTUAL_SPHERE_LAPSE_TIME);
    AnimationPathCallback *animCallbackBwd = new AnimationPathCallback(animationPathScaleBwd,
						0.0, 1.f / ANIM_VIRTUAL_SPHERE_LAPSE_TIME);
    (*xformScaleFwd)->setUpdateCallback(animCallbackFwd);
    (*xformScaleBwd)->setUpdateCallback(animCallbackBwd);

    /* apply shaders to geode stateset */
    stateset = new StateSet();
    stateset->setMode(GL_BLEND, StateAttribute::OVERRIDE | StateAttribute::ON );
    stateset->setRenderingHint(StateSet::TRANSPARENT_BIN);
    //sphereGeode->setStateSet(stateset);
    sphereDrawable->setStateSet(stateset);

    Program* shaderProg = new Program;
    stateset->setAttribute(shaderProg);
    shaderProg->addShader(Shader::readShaderFile(Shader::VERTEX, ANIMDataDir() + "Shaders/VirtualSphere.vert"));
    shaderProg->addShader(Shader::readShaderFile(Shader::FRAGMENT, ANIMDataDir() + "Shaders/VirtualSphere.frag"));

    Image* envMap = osgDB::readImageFile(ANIMDataDir() + "Textures/EnvMap.JPG");
    Texture2D* envTex = new Texture2D(envMap);
    stateset->setTextureAttributeAndModes(0, envTex, StateAttribute::ON);

    Uniform* envMapSampler = new Uniform("EnvMap", 0);
    stateset->addUniform(envMapSampler);

    Uniform* baseColorUniform = new Uniform("BaseColor", Vec3(0.2, 1.0, 0.2));
    stateset->addUniform(baseColorUniform);

    Uniform* lightPosUniform = new Uniform("LightPos", Vec4(1.0, 0.0, 0.2, 0.0));
    stateset->addUniform(lightPosUniform);

}


};

