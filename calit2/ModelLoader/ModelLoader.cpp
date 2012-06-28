#include "ModelLoader.h"

#include <cvrConfig/ConfigManager.h>
#include <cvrKernel/SceneManager.h>
#include <cvrMenu/MenuSystem.h>
#include <cvrKernel/PluginHelper.h>
#include <cvrUtil/TextureVisitors.h>

#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

#include <osg/Matrix>
#include <osg/CullFace>
#include <osgDB/ReadFile>

using namespace osg;
using namespace std;
using namespace cvr;

CVRPLUGIN(ModelLoader)

ModelLoader::ModelLoader() : FileLoadCallback("iv,wrl,vrml,obj,osg,earth")
{
}

bool ModelLoader::init()
{
    std::cerr << "ModelLoader init\n";

    MLMenu = new SubMenu("ModelLoader", "ModelLoader");
    MLMenu->setCallback(this);

    loadMenu = new SubMenu("Load","Load");
    loadMenu->setCallback(this);
    MLMenu->addItem(loadMenu);

    removeButton = new MenuButton("Remove All");
    removeButton->setCallback(this);
    MLMenu->addItem(removeButton);

    vector<string> list;

    string configBase = "Plugin.ModelLoader.Files";

    ConfigManager::getChildren(configBase,list);

    for(int i = 0; i < list.size(); i++)
    {
	MenuButton * button = new MenuButton(list[i]);
	button->setCallback(this);
	menuFileList.push_back(button);

	struct loadinfo * info = new struct loadinfo;
	info->name = list[i];
	info->path = ConfigManager::getEntry("path",configBase + "." + list[i],"");
	info->mask = ConfigManager::getInt("mask",configBase + "." + list[i], 1);
	info->lights = ConfigManager::getInt("lights",configBase + "." + list[i], 1);
	info->backfaceCulling = ConfigManager::getBool("backfaceCulling",configBase + "." + list[i], false);
	info->showBound = ConfigManager::getBool("showBound",configBase + "." + list[i], false);

	models.push_back(info);
    }

    configPath = ConfigManager::getEntry("Plugin.ModelLoader.ConfigDir");

    ifstream cfile;
    cfile.open((configPath + "/Init.cfg").c_str(), ios::in);

    if(!cfile.fail())
    {
	string line;
	while(!cfile.eof())
	{
	    Matrix m;
	    float scale;
	    char name[150];
	    cfile >> name;
	    if(cfile.eof())
	    {
		break;
	    }
	    cfile >> scale;
	    for(int i = 0; i < 4; i++)
	    {
		for(int j = 0; j < 4; j++)
		{
		    cfile >> m(i, j);
		}
	    } 
	    locInit[string(name)] = pair<float, Matrix>(scale, m);
	}
    }
    cfile.close();

    for(int k = 0; k < menuFileList.size(); k++)
    {
	loadMenu->addItem(menuFileList[k]);
    }

    MenuSystem::instance()->addMenuItem(MLMenu);

    std::cerr << "ModelLoader init done.\n";
    return true;
}


ModelLoader::~ModelLoader()
{
}

void ModelLoader::menuCallback(MenuItem* menuItem)
{
    if(menuItem == removeButton)
    {
	    std::map<SceneObject*,MenuButton*>::iterator it;
	    for(it = _saveMap.begin(); it != _saveMap.end(); it++)
	    {
	        delete it->second;
	    }
	    for(it = _loadMap.begin(); it != _loadMap.end(); it++)
	    {
	        delete it->second;
	    }
	    for(it = _resetMap.begin(); it != _resetMap.end(); it++)
	    {
	        delete it->second;
	    }
	    for(it = _deleteMap.begin(); it != _deleteMap.end(); it++)
	    {
	        delete it->second;
	    }
	    for(std::map<SceneObject*,SubMenu*>::iterator it2 = _posMap.begin(); it2 != _posMap.end(); it2++)
	    {
	        delete it->second;
	    }
	    for(std::map<SceneObject*,SubMenu*>::iterator it2 = _saveMenuMap.begin();
		    it2 != _saveMenuMap.end(); it2++)
	    {
	        delete it->second;
	    }
	    _saveMap.clear();
	    _loadMap.clear();
	    _resetMap.clear();
	    _deleteMap.clear();
	    _posMap.clear();
	    _saveMenuMap.clear();

	    for(int i = 0; i < _loadedObjects.size(); i++)
	    {
	        delete _loadedObjects[i];
	    }
	    _loadedObjects.clear();

	    return;
    }

    for(int i = 0; i < menuFileList.size(); i++)
    {
	    if(menuFileList[i] == menuItem)
	    {
	        if (!isFile(models[i]->path.c_str()))
	        {
	            std::cerr << "ModelLoader: file not found: " << models[i]->path << endl;
		        return;
	        }
	
	        // Prepare scene object for model file(s):
	        SceneObject * so;
	        so = new SceneObject(models[i]->name, false, false, false, true, models[i]->showBound);
	        osg::Switch* switchNode = new osg::Switch();
	        so->addChild(switchNode);
	        PluginHelper::registerSceneObject(so,"ModelLoader");
	        so->attachToScene();

            char filename[256];
            strcpy(filename, models[i]->path.c_str());
            bool done = false;
            while (!done)
            {
                // Read model file:
                std::string filenamestring(filename);
                osg::Node* modelNode = osgDB::readNodeFile(filenamestring);
                if(modelNode==NULL)
                { 
                    std::cerr << "ModelLoader: Error reading file " << filename << endl;
                    done=true;
                    break;
                }
                if(models[i]->mask)
                {
                    modelNode->setNodeMask(modelNode->getNodeMask() & ~2);
                }
                if(!models[i]->lights)
                {
                    osg::StateSet* stateset = modelNode->getOrCreateStateSet();
                    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
                }
                if(models[i]->backfaceCulling)
                {
	                osg::StateSet * stateset = modelNode->getOrCreateStateSet();
	                osg::CullFace * cf=new osg::CullFace();
	                cf->setMode(osg::CullFace::BACK);
    	            stateset->setAttributeAndModes( cf, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                }
                
                // TODO: This should be configurable in menu or config file
                TextureResizeNonPowerOfTwoHintVisitor tr2v(false);
                modelNode->accept(tr2v);
                
                switchNode->addChild(modelNode);
           
                if (!increaseFilename(filename)) done=true;
                else 
                {
                    cerr << "ModelLoader: Checking for file " << filename << endl;
                    if (!isFile(filename)) 
                    {
                      cerr << "ModelLoader: no more time steps found" << endl;
                      done=true;
                    }
                }
            }
            if (switchNode->getNumChildren()==1)
            {
                cerr << "ModelLoader: Model file loaded successfully." << endl;
            }
            else
            {
                cerr << "ModelLoader: Loaded animation with " << switchNode->getNumChildren() << " time steps" << endl;
            }
            switchNode->setSingleChildOn(0);

	        if(locInit.find(models[i]->name) != locInit.end())
	        {
		        osg::Matrix scale;
		        scale.makeScale(osg::Vec3(locInit[models[i]->name].first,locInit[models[i]->name].first,
			            locInit[models[i]->name].first));
		        so->setTransform(scale * locInit[models[i]->name].second);
	        }
	        so->setNavigationOn(true);
	        so->addMoveMenuItem();
	        so->addNavigationMenuItem();

	        SubMenu * sm = new SubMenu("Position");
	        so->addMenuItem(sm);
	        _posMap[so] = sm;

	        MenuButton * mb;
	        mb = new MenuButton("Load");
	        mb->setCallback(this);
	        sm->addItem(mb);
	        _loadMap[so] = mb;

	        SubMenu * savemenu = new SubMenu("Save");
	        sm->addItem(savemenu);
	        _saveMenuMap[so] = savemenu;

	        mb = new MenuButton("Save");
	        mb->setCallback(this);
	        savemenu->addItem(mb);
	        _saveMap[so] = mb;

	        mb = new MenuButton("Reset");
	        mb->setCallback(this);
	        sm->addItem(mb);
	        _resetMap[so] = mb;

	        mb = new MenuButton("Delete");
	        mb->setCallback(this);
	        so->addMenuItem(mb);
	        _deleteMap[so] = mb;

	        _loadedObjects.push_back(so);
	    }
    }

    for(std::map<SceneObject*,MenuButton*>::iterator it = _saveMap.begin(); it != _saveMap.end(); it++)
    {
	    if(menuItem == it->second)
	    {
	        std::cerr << "Save." << std::endl;
	        bool nav;
	        nav = it->first->getNavigationOn();
	        it->first->setNavigationOn(false);

	        locInit[it->first->getName()] = std::pair<float, osg::Matrix>(1.0,it->first->getTransform());

	        it->first->setNavigationOn(nav);

	        writeConfigFile(); 
	    }
    }

    for(std::map<SceneObject*,MenuButton*>::iterator it = _loadMap.begin(); it != _loadMap.end(); it++)
    {
	if(menuItem == it->second)
	{
	    bool nav;
	    nav = it->first->getNavigationOn();
	    it->first->setNavigationOn(false);

	    if(locInit.find(it->first->getName()) != locInit.end())
	    {
		std::cerr << "Load." << std::endl;
		//osg::Matrix scale;
		//scale.makeScale(osg::Vec3(locInit[it->first->getName()].first,
		//                locInit[it->first->getName()].first,locInit[it->first->getName()].first));
		it->first->setTransform(locInit[it->first->getName()].second);
	    }

	    it->first->setNavigationOn(nav);
	}
    }

    for(std::map<SceneObject*,MenuButton*>::iterator it = _resetMap.begin(); it != _resetMap.end(); it++)
    {
	if(menuItem == it->second)
	{
	    bool nav;
	    nav = it->first->getNavigationOn();
	    it->first->setNavigationOn(false);

	    if(locInit.find(it->first->getName()) != locInit.end())
	    {
		it->first->setTransform(osg::Matrix::identity());
	    }

	    it->first->setNavigationOn(nav);
	}
    }

    for(std::map<SceneObject*,MenuButton*>::iterator it = _deleteMap.begin(); it != _deleteMap.end(); it++)
    {
	if(menuItem == it->second)
	{
	    if(_saveMap.find(it->first) != _saveMap.end())
	    {
		delete _saveMap[it->first];
		_saveMap.erase(it->first);
	    }
	    if(_loadMap.find(it->first) != _loadMap.end())
	    {
		delete _loadMap[it->first];
		_loadMap.erase(it->first);
	    }
	    if(_resetMap.find(it->first) != _resetMap.end())
	    {
		delete _resetMap[it->first];
		_resetMap.erase(it->first);
	    }
	    if(_posMap.find(it->first) != _posMap.end())
	    {
		delete _posMap[it->first];
		_posMap.erase(it->first);
	    }
	    if(_saveMenuMap.find(it->first) != _saveMenuMap.end())
	    {
		delete _saveMenuMap[it->first];
		_saveMenuMap.erase(it->first);
	    }
	    for(std::vector<SceneObject*>::iterator delit = _loadedObjects.begin();
		    delit != _loadedObjects.end();
		    delit++)
	    {
		if((*delit) == it->first)
		{
		    _loadedObjects.erase(delit);
		    break;
		}
	    }

	    delete it->first;
	    delete it->second;
	    _deleteMap.erase(it);

	    break;
	}
    }
}

/** Copies the tail string after the last occurrence of a given character.
    Example: str="c:\ local\ testfile.dat", c='\' => suffix="testfile.dat"
    @param suffix <I>allocated</I> space for the found string
    @param str    source string
    @param c      character after which to copy characters
    @return result in suffix, empty string if c was not found in str
*/
void ModelLoader::strcpyTail(char* suffix, const char* str, char c)
{
  int i, j;

  // Search for c in pathname:
  i = strlen(str) - 1;
  while (i>=0 && str[i]!=c)
    --i;

  // Extract tail string:
  if (i<0)                                        // c not found?
  {
    //strcpy(suffix, str);
    strcpy(suffix, "");
  }
  else
  {
    for (j=i+1; j<(int)strlen(str); ++j)
      suffix[j-i-1] = str[j];
    suffix[j-i-1] = '\0';
  }
}

/** Extracts a filename from a given path.
    Directory elements have to be separated by '/' or '\' depending on OS.
    @param filename <I>allocated</I> space for filename (e.g. "testfile.dat")
    @param pathname file including entire path (e.g. "/usr/local/testfile.dat")
    @return result in filename
*/
void ModelLoader::extractFilename(char* filename, const char* pathname)
{
  if (strchr(pathname, '/')) strcpyTail(filename, pathname, '/');
  else strcpy(filename, pathname);
}

/** Extracts an extension from a given path or filename.
    @param extension <I>allocated</I> space for extension (e.g. "dat")
    @param pathname  file including entire path (e.g. "/usr/local/testfile.dat")
    @return result in extension
*/
void ModelLoader::extractExtension(char* extension, const char* pathname)
{
  char *filename = new char[strlen(pathname)+1];
  extractFilename(filename, pathname);

  strcpyTail(extension, filename, '.');
  delete[] filename;
}

/** Increases the filename (filename must include an extension!).
  @return true if successful, false if filename couldn't be increased.
          Does not check if the file with the increased name exists.
*/
bool ModelLoader::increaseFilename(char* filename)
{
  bool done = false;
  int i;
  char ext[256];

  extractExtension(ext, filename);
  if (strlen(ext)==0) i=strlen(filename) - 1;
  else i = strlen(filename) - strlen(ext) - 2;
  while (!done)
  {
    if (i<0 || filename[i]<'0' || filename[i]>'9')
      return false;

    if (filename[i] == '9')                       // overflow?
    {
      filename[i] = '0';
      --i;
    }
    else
    {
      ++filename[i];
      done = 1;
    }
  }
  return true;
}

/** Checks if a file exists.
    @param filename file name to check for
    @return true if file exists
*/
bool ModelLoader::isFile(const char* filename)
{
  struct stat buf;
  if (stat(filename, &buf) == 0)
  {
    if (S_ISREG(buf.st_mode)) return true;
  }
  return false;
}

void ModelLoader::preFrame()
{
    static unsigned int counter = 0;
    unsigned int numChildren;
    
    // Switch time dependent objects to next step:
    for(int i = 0; i < _loadedObjects.size(); i++)
    {
        osg::Switch* switchnode = dynamic_cast<osg::Switch*>(_loadedObjects[i]->getChildNode(0));
        if(!switchnode)
	{
	    std::cerr << "ModelLoader: Unable to get model Switch node." << std::endl;
	    continue;
	}
        numChildren = switchnode->getNumChildren();
        if (numChildren > 1)
        {
            switchnode->setAllChildrenOff();
            switchnode->setSingleChildOn(counter % numChildren);
        }
    }
    
    // TODO: add delay for this:
    ++counter;
}

bool ModelLoader::loadFile(std::string file)
{
    std::cerr << "ModelLoader: Loading file: " << file << std::endl;

    osg::Node* modelNode = osgDB::readNodeFile(file);
    if(modelNode==NULL)
    {
	cerr << "ModelLoader: Error reading file " << file << endl;
	return false;
    }
    else
    {
	modelNode->setNodeMask(modelNode->getNodeMask() & ~2);
    }

    std::string name;

    size_t pos = file.find_last_of("/\\");
    if(pos == std::string::npos)
    {
	name = file;
    }
    else
    {
	if(pos + 1 < file.length())
	{
	    name = file.substr(pos+1);
	}
	else
	{
	    name = "Loaded File";
	}
    }

    TextureResizeNonPowerOfTwoHintVisitor tr2v(false);
    modelNode->accept(tr2v);

    SceneObject * so = new SceneObject(name,false,false,false,true,false);
    PluginHelper::registerSceneObject(so,"ModelLoader");
    so->addChild(modelNode);
    so->attachToScene();
    so->setNavigationOn(true);
    so->addMoveMenuItem();
    so->addNavigationMenuItem();

    MenuButton * mb;

    mb = new MenuButton("Reset Position");
    mb->setCallback(this);
    so->addMenuItem(mb);
    _resetMap[so] = mb;

    mb = new MenuButton("Delete");
    mb->setCallback(this);
    so->addMenuItem(mb);
    _deleteMap[so] = mb;

    _loadedObjects.push_back(so);

    return true;
}

void ModelLoader::writeConfigFile()
{
    ofstream cfile;
    cfile.open((configPath + "/Init.cfg").c_str(), ios::trunc);

    if(!cfile.fail())
    {
	for(map<std::string, std::pair<float, osg::Matrix> >::iterator it = locInit.begin();
		it != locInit.end(); it++)
	{
	    //cerr << "Writing entry for " << it->first << endl;
	    cfile << it->first << " " << it->second.first << " ";
	    for(int i = 0; i < 4; i++)
	    {
		for(int j = 0; j < 4; j++)
		{
		    cfile << it->second.second(i, j) << " ";
		}
	    }
	    cfile << endl;
	}
    }
    cfile.close();
}
