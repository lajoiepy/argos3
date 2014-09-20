/**
 * @file <argos3/core/utility/plugins/dynamic_loading.cpp>
 *
 * @author Carlo Pinciroli - <ilpincy@gmail.com>
 */

#include "dynamic_loading.h"

#include <dirent.h>
#include <cerrno>

namespace argos {

   /****************************************/
   /****************************************/

   CDynamicLoading::TDLHandleMap CDynamicLoading::m_tOpenLibs;
   const std::string CDynamicLoading::DEFAULT_PLUGIN_PATH = ARGOS_INSTALL_PREFIX "/lib/argos3/";

   /****************************************/
   /****************************************/

   /*
    * Tries to load the given library
    * 1. Tries to load the library as passed
    * 2. If that fails, it appends the shared library extension and tries again;
    * 3. If also that fails, it appends the module library extension and tries a last time.
    * If all fails, t_handle is set to NULL and str_lib is left as-is;
    * In case of success, it sets t_handle to the handle of the load library, and fixes str_lib to
    * match the extension of the loaded library.
    */
   static CDynamicLoading::TDLHandle LoadLibraryTryingExtensions(std::string& str_lib) {
      /* Try loading without changes to the given path */
      CDynamicLoading::TDLHandle tHandle = ::dlopen(str_lib.c_str(), RTLD_LAZY);
      if(tHandle == NULL) {
         LOGERR << "[WARNING] Can't open library \"" << str_lib << "\": " << ::dlerror() << std::endl;
         LOGERR.Flush();
         /* Try adding the shared lib extension to the path */
         std::string strLibWExt = str_lib + "." + ARGOS_SHARED_LIBRARY_EXTENSION;
         tHandle = ::dlopen(strLibWExt.c_str(), RTLD_LAZY);
         if(tHandle != NULL) {
            /* Success */
            str_lib = strLibWExt;
         }
         else {
            LOGERR << "[WARNING] Can't open library \"" << strLibWExt << "\": " << ::dlerror() << std::endl;
            LOGERR.Flush();
            /* Try adding the module lib extension to the path */
            strLibWExt = str_lib + "." + ARGOS_MODULE_LIBRARY_EXTENSION;
            tHandle = ::dlopen(strLibWExt.c_str(), RTLD_LAZY);
            if(tHandle != NULL) {
               /* Success */
               str_lib = strLibWExt;
            }
            else {
               LOGERR << "[WARNING] Can't open library \"" << strLibWExt << "\": " << ::dlerror() << std::endl;
               LOGERR.Flush();
            }
         }
      }
      return tHandle;
   }

   /****************************************/
   /****************************************/

   CDynamicLoading::TDLHandle CDynamicLoading::LoadLibrary(const std::string& str_lib) {
      TDLHandle tHandle;
      /* Check if the provided path is absolute or relative */
      if(str_lib[0] == '/') {
         /*
          * Absolute path
          */
         /* First check if the library is already loaded */
         TDLHandleMap::iterator it = m_tOpenLibs.find(str_lib);
         if(it != m_tOpenLibs.end()) {
            /* Already loaded */
            return m_tOpenLibs[str_lib];
         }
         /* Not already loaded, load the library and bomb out in case of failure */
         std::string strLoadedLib = str_lib;
         tHandle = LoadLibraryTryingExtensions(strLoadedLib);
         if(tHandle == NULL) {
            THROW_ARGOSEXCEPTION("Can't load library \""
                                 << str_lib
                                 << "\"");
         }
         /* Store the handle to the loaded library */
         m_tOpenLibs[strLoadedLib] = tHandle;
         LOG << "[INFO] Loaded library \"" << strLoadedLib << "\"" << std::endl;
         LOG.Flush();
         return tHandle;
      }
      else {
         /*
          * Relative path, go through the plugin directories
          */
         /* String to store the full path to a library */
         std::string strLibPath;
         /* String to store the list of paths to search */
         std::string strPluginPath = ".:" + DEFAULT_PLUGIN_PATH;
         /* Get variable ARGOS_PLUGIN_PATH from the environment */
         if(::getenv("ARGOS_PLUGIN_PATH") != NULL) {
            /* Add value of the variable to list of paths to check */
            strPluginPath += ":" + std::string(::getenv("ARGOS_PLUGIN_PATH"));
         }
         /* Add : at the end to make parsing easier */
         if(strPluginPath[strPluginPath.length()-1] != ':') {
            strPluginPath.append(":");
         }
         /*
          * Go through paths and try to load the library
          */
         /* Parse the string */
         std::istringstream issPluginPath(strPluginPath);
         std::string strDir;
         while(std::getline(issPluginPath, strDir, ':')) {
            /* Add '/' to dir if missing */
            if(strDir[strDir.length()-1] != '/') {
               strDir.append("/");
            }
            strLibPath = strDir + str_lib;
            /* First check if the library is already loaded */
            TDLHandleMap::iterator it = m_tOpenLibs.find(strLibPath);
            if(it != m_tOpenLibs.end()) {
               /* Already loaded */
               return m_tOpenLibs[strLibPath];
            }
            /* Not already loaded, try and load the library */
            tHandle = LoadLibraryTryingExtensions(strLibPath);
            if(tHandle != NULL) {
               /* Store the handle to the loaded library */
               m_tOpenLibs[strLibPath] = tHandle;
               LOG << "[INFO] Loaded library \"" << strLibPath << "\"" << std::endl;
               LOG.Flush();
               return tHandle;
            }
         }
         /* If we get here, it's because no directory worked */
         THROW_ARGOSEXCEPTION("Can't load library \""
                              << str_lib
                              << "\".");
      }
   }

   /****************************************/
   /****************************************/

   void CDynamicLoading::UnloadLibrary(const std::string& str_lib) {
      TDLHandleMap::iterator it = m_tOpenLibs.find(str_lib);
      if(it != m_tOpenLibs.end()) {
         UnloadLibrary(it->second);
      }
      else {
         THROW_ARGOSEXCEPTION("Can't unload library \""
                              << str_lib
                              << "\": library does not appear to have been loaded.");
      }
   }

   /****************************************/
   /****************************************/

   void CDynamicLoading::UnloadLibrary(TDLHandle t_lib) {
      ::dlclose(t_lib);
   }

   /****************************************/
   /****************************************/

   void CDynamicLoading::LoadAllLibraries() {      
      /* String to store the full path to a library */
      std::string strLibPath;
      /* String to store the list of paths to search */
      std::string strPluginPath = DEFAULT_PLUGIN_PATH;
      /* Get variable ARGOS_PLUGIN_PATH from the environment */
      if(::getenv("ARGOS_PLUGIN_PATH") != NULL) {
         /* Add value of the variable to list of paths to check */
         strPluginPath += ":" + std::string(::getenv("ARGOS_PLUGIN_PATH"));
      }
      /* Add : at the end to make parsing easier */
      if(strPluginPath[strPluginPath.length()-1] != ':') {
         strPluginPath.append(":");
      }
      /*
       * Go through paths and load all the libraries
       */
      /* Directory info */
      DIR* ptDir;
      struct dirent* ptDirData;
      /* Parse the string */
      std::istringstream issPluginPath(strPluginPath);
      std::string strDir;
      while(std::getline(issPluginPath, strDir, ':')) {
         /* Add '/' to dir if missing */
         if(strDir[strDir.length()-1] != '/') {
            strDir.append("/");
         }
         /* Try to open the directory */
         ptDir = ::opendir(strDir.c_str());
         if(ptDir != NULL) {
            /* Directory open, now go through the files in the directory */
            while((ptDirData = ::readdir(ptDir)) != NULL) {
               /* We have a file, check that it is a library file */
               if(std::string(ptDirData->d_name).rfind("." ARGOS_SHARED_LIBRARY_EXTENSION) != std::string::npos) {
                  /* It's a library file, load it */
                  LoadLibrary(strDir + ptDirData->d_name);
               }
               if(std::string(ARGOS_SHARED_LIBRARY_EXTENSION) != std::string(ARGOS_MODULE_LIBRARY_EXTENSION)) {
                  if(std::string(ptDirData->d_name).rfind("." ARGOS_MODULE_LIBRARY_EXTENSION) != std::string::npos) {
                     /* It's a library file, load it */
                     LoadLibrary(strDir + ptDirData->d_name);
                  }
               }
            }
            /* Close directory */
            ::closedir(ptDir);
         }
         else {
            /* Error opening directory open, inform user without bombing out */
            LOGERR << "[WARNING] Error opening directory \""
                   << strDir
                   << "\": "
                   << ::strerror(errno)
                   << std::endl;
            LOGERR.Flush();
         }
      }
   }

   /****************************************/
   /****************************************/

   void CDynamicLoading::UnloadAllLibraries() {
      for(TDLHandleMap::iterator it = m_tOpenLibs.begin();
          it != m_tOpenLibs.end();
          ++it) {
         UnloadLibrary(it->second);
      }
   }

   /****************************************/
   /****************************************/

}
