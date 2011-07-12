#include "GreenLight.h"

#include <kernel/PluginHelper.h>
#include <osg/Material>
#include <osg/StateSet>

GreenLight::Entity::Entity(osg::Node * node, osg::Matrix mat)
{
    transform = new osg::MatrixTransform(mat);
    mainNode = node;
    transform->addChild(mainNode);
    createNodeSet(mainNode);
    status = START;
    time = 0;
    minWattage = 0;
    maxWattage = 0;
}

void GreenLight::Entity::handleAnimation()
{
    if (status == FORWARD)
    {
        time += cvr::PluginHelper::getLastFrameDuration();

        if (time > path->getPeriod())
        {
            time = path->getPeriod();
            status = END;
        }

        osg::Matrix aniMatrix;
        path->getMatrix(time,aniMatrix);
        transform->setMatrix(aniMatrix);
    }
    else if (status == REVERSE)
    {
        time -= cvr::PluginHelper::getLastFrameDuration();

        if (time < 0)
        {
            time = 0;
            status = START;
        }

        osg::Matrix aniMatrix;
        path->getMatrix(time,aniMatrix);
        transform->setMatrix(aniMatrix);
    }
}

void GreenLight::Entity::beginAnimation()
{
   if (!path)
       return;
   if (status == START || status == REVERSE)
       status = FORWARD;
   else if (status == END || status == FORWARD)
       status = REVERSE;
}

void GreenLight::Entity::createNodeSet(osg::Node * node)
{
    if (node->asGeode())
        nodes.insert(node);

    osg::Group * group = node->asGroup();
    if (group)
    {
        for (int i = 0; i < group->getNumChildren(); i++)
            createNodeSet(group->getChild(i));
    }
}


void GreenLight::Entity::setTransparency(bool transparent, bool onTransform)
{
	float level = transparent ? 0.1f : 1.0f;

        osg::ref_ptr<osg::StateSet> stateset;
        
        if (onTransform)
            stateset = transform->getStateSet();
        else
            stateset = mainNode->getOrCreateStateSet();

        osg::ref_ptr<osg::Material> mm = dynamic_cast<osg::Material*>(stateset->getAttribute
            (osg::StateAttribute::MATERIAL));

        if (!mm)
            mm = new osg::Material;

        mm->setAlpha(osg::Material::FRONT_AND_BACK, level);

        stateset->setMode(GL_BLEND, osg::StateAttribute::OVERRIDE |
            osg::StateAttribute::ON );
        stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        stateset->setAttributeAndModes( mm, osg::StateAttribute::OVERRIDE |
            osg::StateAttribute::ON);

        if (onTransform)
            transform->setStateSet(stateset);
        else
            mainNode->setStateSet(stateset);
}

void GreenLight::Entity::setColor(const osg::Vec3 color)
{
    osg::ref_ptr<osg::StateSet> stateset = transform->getStateSet();
    osg::ref_ptr<osg::Material> mm = dynamic_cast<osg::Material*>(stateset->getAttribute
        (osg::StateAttribute::MATERIAL));

    osg::Vec4 color4;
    color4 = mm->getDiffuse(osg::Material::FRONT_AND_BACK);
    color4 = osg::Vec4(color, color4.w());
    mm->setDiffuse(osg::Material::FRONT_AND_BACK, color4);

    stateset->setAttributeAndModes( mm, osg::StateAttribute::OVERRIDE |
            osg::StateAttribute::ON );

    transform->setStateSet(stateset);
}

void GreenLight::Entity::addChild(Entity * child)
{
    if (!child)
    {
        std::cerr << "Error: NULL child parameter passed to GreenLight::Entity::addChild function." << std::endl;
        return;
    }

    transform->addChild(child->transform);
}

void GreenLight::Entity::showVisual(bool show)
{
    if (transform->containsNode(mainNode))
    {
        transform->removeChild(mainNode);
    }
    else
    {
        transform->addChild(mainNode);
    }
}

void GreenLight::Entity::setDefaultMaterial()
{
    osg::ref_ptr<osg::StateSet> stateset = transform->getOrCreateStateSet();
    osg::ref_ptr<osg::Material> mm = dynamic_cast<osg::Material*>(stateset->getAttribute
        (osg::StateAttribute::MATERIAL));

    if (!mm)
        mm = new osg::Material;

    mm->setColorMode(osg::Material::DIFFUSE);
    mm->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(.7,.7,.7,1));

    stateset->setMode(GL_LIGHTING, osg::StateAttribute::PROTECTED |
            osg::StateAttribute::OFF );
    stateset->setAttributeAndModes( mm, osg::StateAttribute::OVERRIDE |
            osg::StateAttribute::ON );

    transform->setStateSet(stateset);
}