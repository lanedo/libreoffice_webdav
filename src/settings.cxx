#include "settings.hxx"
#include <cstdio>
#include <osl/diagnose.h>
#include <rtl/ustring.hxx>
#include <cppuhelper/implbase1.hxx>
#include <com/sun/star/awt/Key.hpp>
#include <com/sun/star/awt/XDialogProvider2.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/lang/XComponent.hpp>

using rtl::OUString;
using namespace css::awt;
using namespace css::beans;
using namespace css::container;
using namespace css::lang;
using namespace css::uno;
using css::lang::XMultiComponentFactory;
using css::awt::Key::RETURN;

namespace WebDAVUI {

Settings::Settings (const Reference< css::uno::XComponentContext > &rxContext) : mxContext (rxContext)
{
    printf ("Settings::Settings\n");
    mxMCF = mxContext->getServiceManager ();
    Reference< css::lang::XMultiServiceFactory > multiServiceFactory (
        mxContext->getServiceManager(), UNO_QUERY_THROW);
    loadSettings (multiServiceFactory);
}

bool Settings::getStringValueByReference (Reference<css::container::XNameAccess>& xAccess,
    const OUString& aKeyName, OUString& aValue)
{
    OSL_ASSERT(xAccess.is());
    xAccess->getByName(aKeyName) >>= aValue;
    printf ("Settings::getStringValueByReference %s → '%s'\n",
        OUStringToOString( aKeyName, RTL_TEXTENCODING_ASCII_US ).getStr(),
        OUStringToOString( aValue, RTL_TEXTENCODING_ASCII_US ).getStr());
    return aValue.getLength() != 0;
}

OUString Settings::getStringValue (const OUString& aKeyName)
{
    OUString aValue;
    Reference<css::container::XNameAccess > xChildAccess(mxIface, UNO_QUERY_THROW);
    getStringValueByReference (xChildAccess, aKeyName, aValue);
    return aValue;
}

OUString Settings::getRemoveServerName ()
{
    return getStringValue (OUString::createFromAscii ("ooInetHTTPProxyName"));
}

bool Settings::loadSettings (Reference< css::lang::XMultiServiceFactory > const & factory)
{
    printf ("Settings::loadSettings\n");
    OSL_ASSERT (factory.is());
    const OUString kConfigurationProviderService (
        RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.configuration.ConfigurationProvider"));
    const OUString kReadOnlyViewService (
        RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.configuration.ConfigurationAccess"));
    const OUString kComponent (RTL_CONSTASCII_USTRINGPARAM("org.openoffice.Inet/Settings"));
    const OUString kServerDefinition (RTL_CONSTASCII_USTRINGPARAM ("ooInetProxyType"));

    try
    {
        Reference< css::lang::XMultiServiceFactory > xCfgProvider (
            factory->createInstance (kConfigurationProviderService), UNO_QUERY);
        OSL_ENSURE (xCfgProvider.is (), "Failed to create ConfigurationProvider");
        if (!xCfgProvider.is())
            return false;

        css::beans::NamedValue aPath (OUString (RTL_CONSTASCII_USTRINGPARAM ("nodepath")),
                                      makeAny (kComponent) );
        Sequence< Any > aArgs (1);
        aArgs[0] <<=  aPath;
        mxIface = xCfgProvider->createInstanceWithArguments (kReadOnlyViewService, aArgs);
        Reference<css::container::XNameAccess > xAccess (mxIface, UNO_QUERY_THROW);
        xAccess->getByName (kServerDefinition) >>= mxIface;
    }
    catch (Exception & e)
    {
        printf ("Failed to instantiate XNameAccess: %s\n",
                OUStringToOString (e.Message, RTL_TEXTENCODING_ASCII_US).getStr ());
        return false;
    }

    return true;
}

}

