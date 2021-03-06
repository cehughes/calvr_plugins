/***************************************************************
* File Name: DSTexturePallette.cpp
*
* Description: 
*
* Written by ZHANG Lelin on Jan 12, 2011
*
***************************************************************/
#include "DSTexturePallette.h"

using namespace std;
using namespace osg;


// Constructor
DSTexturePallette::DSTexturePallette(): mTexIndex(0), mTexturingState(IDLE), mAudioConfigHandler(NULL)
{
    // load objects for design state root switch
    CAVEAnimationModeler::ANIMLoadTexturePalletteRoot(&mPATransFwd, &mPATransBwd);

    // load objects for 'IDLE' state
    CAVEAnimationModeler::ANIMLoadTexturePalletteIdle(&mIdleStateSwitch, &mTextureStatesIdleEntry);

    // load objects for 'SELECT_TEXTURE' and 'APPLY_TEXTURE' state
    CAVEAnimationModeler::ANIMLoadTexturePalletteSelect(&mSelectStateSwitch, &mAlphaTurnerSwitch, 
							mNumTexs, &mTextureStatesSelectEntryArray);

    this->addChild(mPATransFwd);
    this->addChild(mPATransBwd);

    mPATransFwd->addChild(mIdleStateSwitch);		mPATransBwd->addChild(mIdleStateSwitch);
    mPATransFwd->addChild(mSelectStateSwitch);		mPATransBwd->addChild(mSelectStateSwitch);
    mPATransFwd->addChild(mAlphaTurnerSwitch);		mPATransBwd->addChild(mAlphaTurnerSwitch);

    // use both instances of intersector
    mDSIntersector = new DSIntersector();
    mDOIntersector = new DOIntersector();
    mDSIntersector->loadRootTargetNode(NULL, NULL);
    mDOIntersector->loadRootTargetNode(NULL, NULL);

    setAllChildrenOff();
    mIdleStateSwitch->setAllChildrenOn(); // default state = IDLE
    mSelectStateSwitch->setAllChildrenOff();
    mAlphaTurnerSwitch->setAllChildrenOff();

    mDevPressedFlag = false;
    mIsOpen = false;
}


// Destructor
DSTexturePallette::~DSTexturePallette()
{
}


/***************************************************************
* Function: setObjectEnabled()
*
* Description:
*
***************************************************************/
void DSTexturePallette::setObjectEnabled(bool flag)
{
    //std::cout << "setEnabled" << std::endl;
    mObjEnabledFlag = flag;

    if (flag) setAllChildrenOn();
    if (!mPATransFwd || !mPATransBwd) return;

    AnimationPathCallback* animCallback = NULL;
    if (flag)
    {
        if (!mIsOpen)
        {
            this->setSingleChildOn(0);
            animCallback = dynamic_cast <AnimationPathCallback*> (mPATransFwd->getUpdateCallback());
            mSelectStateSwitch->setAllChildrenOn();

            for (int i = 0; i < mNumTexs; i++)
            {
                mTextureStatesSelectEntryArray[i]->mStateAnimationArray[0]->reset();
                mTextureStatesSelectEntryArray[i]->mEntrySwitch->setSingleChildOn(0);
            }
            resetIntersectionRootTarget();
            mIsOpen = true;
            mTexturingState = SELECT_TEXTURE;
        }
        else
        {
            for (int i = 0; i < mNumTexs; i++)
            {
                mTextureStatesSelectEntryArray[i]->mStateAnimationArray[1]->reset();
                mTextureStatesSelectEntryArray[i]->mEntrySwitch->setSingleChildOn(1);
            }
            mTexturingState = IDLE;
            mIsOpen = false;
        }
    } 
    else 
    {
        //texturingStateTransitionHandle(mTexturingState, IDLE);
        //this->setSingleChildOn(1);
        //animCallback = dynamic_cast <AnimationPathCallback*> (mPATransBwd->getUpdateCallback());
        //mDSIntersector->loadRootTargetNode(NULL, NULL);
        //mDOIntersector->loadRootTargetNode(NULL, NULL);

        mTexturingState = IDLE;
        for (int i = 0; i < mNumTexs; i++)
        {
            mTextureStatesSelectEntryArray[i]->mStateAnimationArray[1]->reset();
            mTextureStatesSelectEntryArray[i]->mEntrySwitch->setSingleChildOn(1);
        }
        mIsOpen = false;
        resetIntersectionRootTarget();
    }

    if (animCallback) 
        animCallback->reset();
}


/***************************************************************
* Function: switchToPrevSubState()
***************************************************************/
void DSTexturePallette::switchToPrevSubState()
{
//std::cout << "switchPrev" << std::endl;
    // prev state look up 
    switch (mTexturingState)
    {
        case IDLE:
        {
            mTexturingState = APPLY_TEXTURE;
            texturingStateTransitionHandle(IDLE, APPLY_TEXTURE);
            break;
        }
        case SELECT_TEXTURE:
        {
            mTexturingState = IDLE; 
            texturingStateTransitionHandle(SELECT_TEXTURE, IDLE);
            break;
        }
        case APPLY_TEXTURE:
        {
            mTexturingState = SELECT_TEXTURE; 
            texturingStateTransitionHandle(APPLY_TEXTURE, SELECT_TEXTURE);
            break;
        }
        default: break;
    }
    
}


/***************************************************************
* Function: switchToNextSubState()
***************************************************************/
void DSTexturePallette::switchToNextSubState()
{
    //std::cout << "switchNext" << std::endl;
    // next state look up 
    switch (mTexturingState)
    {
        case IDLE:
        {
            mTexturingState = SELECT_TEXTURE;
            texturingStateTransitionHandle(IDLE, SELECT_TEXTURE);
            break;
        }
        case SELECT_TEXTURE:
        {
            mTexturingState = APPLY_TEXTURE;
            texturingStateTransitionHandle(SELECT_TEXTURE, APPLY_TEXTURE);
            break;
        }
        case APPLY_TEXTURE:
        {
            mTexturingState = IDLE;
            texturingStateTransitionHandle(APPLY_TEXTURE, IDLE);
            break;
        }
        default: break;
    }
}


/***************************************************************
* Function: inputDevMoveEvent()
***************************************************************/
void DSTexturePallette::inputDevMoveEvent(const osg::Vec3 &pointerOrg, const osg::Vec3 &pointerPos)
{	
    if (mDevPressedFlag)
    {
    }
    if (!mDevPressedFlag)
    {
    }
}


/***************************************************************
* Function: inputDevPressEvent()
*
* Proceed to next substate when current state is either 'IDLE'
* or 'SELECT_TEXTURE'.
*
***************************************************************/
bool DSTexturePallette::inputDevPressEvent(const osg::Vec3 &pointerOrg, const osg::Vec3 &pointerPos)
{
    //std::cout << "pressEvent" << std::endl;
    mDevPressedFlag = true;

    if (mIsOpen)
    {
//    mTexturingState = IDLE;
//    resetIntersectionRootTarget();
        // test all submenus for intersection
        
        for (int i = 0; i < mNumTexs; ++i)
        {
            mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, mTextureStatesSelectEntryArray[i]->mEntryGeode);
            if (mDSIntersector->test(pointerOrg, pointerPos))
            {
                mTexIndex = i;
                mTexturingState = APPLY_TEXTURE;
                return true;
            }
        }
        resetIntersectionRootTarget();
 
/*        if (mDSIntersector->test(pointerOrg, pointerPos))
        {
            Node *hitNode = mDSIntersector->getHitNode();
            int hitIdx = mTexIndex;
            std::cout << "hit" << std::endl;
            for (int i = 0; i < mNumTexs; i++)
            {
                // clicked a texture sphere, enter apply texture state
                if (hitNode == mTextureStatesSelectEntryArray[i]->mEntryGeode)
                {
                std::cout << "hitnode == mEntryGeode" << std::endl;
                    hitIdx = i;
                    mTexIndex = i;
                    mTexturingState = APPLY_TEXTURE;
                    return true;
                }
            }
            return true;
        }
        */
     
        // test world geometry for intersection
        if (mTexturingState == APPLY_TEXTURE)
        {
            if (mDOIntersector->test(pointerOrg, pointerPos))
            {
                // adjust texture transparency or apply texture to geode
                Node *hitNode = mDOIntersector->getHitNode();
                CAVEGeode *geode = dynamic_cast <CAVEGeode*> (hitNode);
                if (geode)
                {
                    Vec3 diffuse = mTextureStatesSelectEntryArray[mTexIndex]->getDiffuse();
                    Vec3 specular = mTextureStatesSelectEntryArray[mTexIndex]->getSpecular();
                    string filename = mTextureStatesSelectEntryArray[mTexIndex]->getTexFilename();
                    string audioinfo = mTextureStatesSelectEntryArray[mTexIndex]->getAudioInfo();
                    geode->applyColorTexture(diffuse, specular, 1.0f, filename);
                    geode->applyAudioInfo(audioinfo);

                    // update audio parameters 
                    mAudioConfigHandler->updateShapes();
                    mTexturingState = IDLE;
                    /*TexturingState oldState = mTexturingState;
                    mTexturingState = IDLE;
                    resetIntersectionRootTarget();
                    mTexturingState = oldState;
                    */

                    return true;

                }
            }
        }
        return false;
    }
}


/***************************************************************
* Function: inputDevReleaseEvent()
***************************************************************/
bool DSTexturePallette::inputDevReleaseEvent()
{
    mDevPressedFlag = false;

    return false;
}


/***************************************************************
* Function: update()
***************************************************************/
void DSTexturePallette::update()
{
}


/***************************************************************
* Function: texturingStateTransitionHandle()
***************************************************************/
void DSTexturePallette::texturingStateTransitionHandle(const TexturingState& prevState, const TexturingState& nextState)
{
    // mIdleStateSwitch: always on except transition between 'SELECT_TEXTURE' and 'APPLY_TEXTURE' 
    mIdleStateSwitch->setAllChildrenOn();	
    mSelectStateSwitch->setAllChildrenOn();
    mAlphaTurnerSwitch->setAllChildrenOff();

    int idxSelected = -1;	// index of animation that to be reset for selected texture entry
    int idxUnselected = -1;	// index of animation that to be reset for all un-selected texture entry

    // transitions between 'IDLE' and 'SELECT_TEXTURE' 
    if (prevState == IDLE && nextState == SELECT_TEXTURE)
    {
        idxSelected = 0;	idxUnselected = 0;
        mTextureStatesIdleEntry->mEntrySwitch->setSingleChildOn(1);
        mTextureStatesIdleEntry->mBwdAnim->reset();
    }
    else if (prevState == SELECT_TEXTURE && nextState == IDLE)
    {
        idxSelected = 1;	idxUnselected = 1;
        mTextureStatesIdleEntry->mEntrySwitch->setSingleChildOn(0);
        mTextureStatesIdleEntry->mFwdAnim->reset();
    }

    // transitions between 'SELECT_TEXTURE' and 'APPLY_TEXTURE' 
    else if (prevState == SELECT_TEXTURE && nextState == APPLY_TEXTURE)
    {
        idxSelected = 4;	idxUnselected = 2;
        mIdleStateSwitch->setAllChildrenOff();
    }
    else if (prevState == APPLY_TEXTURE && nextState == SELECT_TEXTURE)
    {
        idxSelected = 5;	idxUnselected = 3;
        mIdleStateSwitch->setAllChildrenOff();

    }

    // transitions between 'IDLE' and 'APPLY_TEXTURE' 
    else if (prevState == IDLE && nextState == APPLY_TEXTURE)
    {
        idxSelected = 7;	idxUnselected = -1;
        mTextureStatesIdleEntry->mEntrySwitch->setSingleChildOn(1);
        mTextureStatesIdleEntry->mBwdAnim->reset();
    }
    else if (prevState == APPLY_TEXTURE && nextState == IDLE)
    {
        idxSelected = 6;	idxUnselected = -1;
        mTextureStatesIdleEntry->mEntrySwitch->setSingleChildOn(0);
        mTextureStatesIdleEntry->mFwdAnim->reset();
    }

    // reset animation associated with 'mTextureStatesSelectEntryArray' 
    for (int i = 0; i < mNumTexs; i++)
    {
        if (i == mTexIndex && idxSelected >= 0)
        {
            mTextureStatesSelectEntryArray[mTexIndex]->mEntrySwitch->setSingleChildOn(idxSelected);
            mTextureStatesSelectEntryArray[mTexIndex]->mStateAnimationArray[idxSelected]->reset();
        }
        else if (idxUnselected >= 0)
        {
            mTextureStatesSelectEntryArray[i]->mEntrySwitch->setSingleChildOn(idxUnselected);
            mTextureStatesSelectEntryArray[i]->mStateAnimationArray[idxUnselected]->reset();
        }
    }
    resetIntersectionRootTarget();
}


/***************************************************************
* Function: resetIntersectionRootTarget()
***************************************************************/
void DSTexturePallette::resetIntersectionRootTarget()
{
    if (mTexturingState == IDLE)
    {
        Node *targetNode = mTextureStatesIdleEntry->mEntryGeode;
        mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, targetNode);
    }
    else if (mTexturingState == SELECT_TEXTURE)
    {
        mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, NULL);
    }
    else if (mTexturingState == APPLY_TEXTURE)
    {
        mDOIntersector->loadRootTargetNode(gDesignObjectRootGroup, NULL);
    }
}


void DSTexturePallette::setHighlight(bool isHighlighted, const osg::Vec3 &pointerOrg, const osg::Vec3 &pointerPos) 
{
    if (isHighlighted)
    {
        //std::cout << "texture highlight" << std::endl;
        int idx = -1;
        for (int i = 0; i < mNumTexs; ++i)
        {
            mTextureStatesSelectEntryArray[i]->mEntryGeode->removeDrawable(mSD);
            mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, mTextureStatesSelectEntryArray[i]->mEntryGeode);
            if (mDSIntersector->test(pointerOrg, pointerPos))
            {
                idx = i;
            }
        }
        resetIntersectionRootTarget();

        if (idx == -1)
            return;

        osg::Sphere *sphere = new osg::Sphere();
        mSD = new osg::ShapeDrawable(sphere);
        sphere->setRadius(0.15);
        mSD->setColor(osg::Vec4(1, 1, 1, 0.5));

        mTextureStatesSelectEntryArray[idx]->mEntryGeode->addDrawable(mSD);

        StateSet *stateset = mSD->getOrCreateStateSet();
        //stateset->setMode(GL_BLEND, StateAttribute::OVERRIDE | StateAttribute::ON);
        //stateset->setMode(GL_CULL_FACE, StateAttribute::OVERRIDE | StateAttribute::ON);
        //stateset->setRenderingHint(StateSet::TRANSPARENT_BIN);
    }
    else
    {
        for (int i = 0; i < mNumTexs; ++i)
        {
            mTextureStatesSelectEntryArray[i]->mEntryGeode->removeDrawable(mSD);
        }
    }
    TexturingState oldState = mTexturingState;;
    mTexturingState = IDLE;
    resetIntersectionRootTarget();
    mTexturingState = oldState;
}

