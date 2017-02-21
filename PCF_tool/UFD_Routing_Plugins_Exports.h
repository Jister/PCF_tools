/*=================================================================================================
               Copyright 2013 Siemens Product Lifecycle Management Software Inc.
                                 Unpublished - All Rights Reserved.
===================================================================================================
File description:

    This contains the export symbols useful for exporting entry points from libroute_plugins.dll.


=================================================================================================*/
#ifndef UFD_Routing_Plugins_Exports_H_INCLUDED
#define UFD_Routing_Plugins_Exports_H_INCLUDED

#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#if defined(_WIN32)
#    define ROUTE_PLUGINSEXPORT       __declspec(dllexport)
#    define ROUTE_PLUGINSGLOBAL       extern __declspec(dllexport)
#    define ROUTE_PLUGINSPRIVATE      extern
#elif __GNUC__ >= 4
#    define ROUTE_PLUGINSEXPORT       __attribute__ ((visibility("default")))
#    define ROUTE_PLUGINSGLOBAL       extern __attribute__ ((visibility("default")))
#    define ROUTE_PLUGINSPRIVATE      extern __attribute__ ((visibility("hidden")))
#else
#    define ROUTE_PLUGINSEXPORT
#    define ROUTE_PLUGINSGLOBAL       extern
#    define ROUTE_PLUGINSPRIVATE      extern
#endif

#endif  // UFD_Routing_Plugins_Exports_H_INCLUDED

