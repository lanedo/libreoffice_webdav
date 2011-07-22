/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*************************************************************************
 *
 *  Copyright (C) 2011
 *  Michael Natterer <mitch@lanedo.com>
 *  Kristian Rietveld <kris@lanedo.com>
 *  Christian Dywan <christian@lanedo.com>
 *  Lionel Dricot <lionel.dricot@lanedo.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General
 *  Public License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *************************************************************************/

#include <stdio.h>
#include <rtl/ustring.hxx>
#include <cppuhelper/queryinterface.hxx> // helper for queryInterface() impl
#include <cppuhelper/factory.hxx> // helper for component factory
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/registry/XRegistryKey.hpp>

// include our specific addon header to get access to functions and definitions
#include "addon.hxx"

using namespace rtl;
using namespace osl;
using namespace cppu;
using namespace css::uno;
using namespace css::lang;
using namespace css::registry;

//##################################################################################################
//#### EXPORTED ####################################################################################
//##################################################################################################


/**
 * Gives the environment this component belongs to.
 */
extern "C" void SAL_CALL component_getImplementationEnvironment(const sal_Char ** ppEnvTypeName, uno_Environment ** ppEnv)
{
    printf ("component_getImplementationEnvironment\n");
    *ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

/**
 * This function creates an implementation section in the registry and another subkey
 *
 * for each supported service.
 * @param pServiceManager   the service manager
 * @param pRegistryKey      the registry key
 */
// This method not longer necessary since OOo 3.4 where the component registration was
// was changed to passive component registration. For more details see
// http://wiki.services.openoffice.org/wiki/Passive_Component_Registration
//
extern "C" sal_Bool SAL_CALL component_writeInfo(void * pServiceManager,
                                                 void * pRegistryKey)
 {
    printf ("component_writeInfo\n");

 	if (pRegistryKey)
 	{
 		try
 		{
 			Reference< XRegistryKey > xNewKey(
 				reinterpret_cast< XRegistryKey * >( pRegistryKey )->createKey(
 					OUString( RTL_CONSTASCII_USTRINGPARAM("/" IMPLEMENTATION_NAME "/UNO/SERVICES") ) ) );

 			const Sequence< OUString > & rSNL =
 				Addon_getSupportedServiceNames();
 			const OUString * pArray = rSNL.getConstArray();
 			for ( sal_Int32 nPos = rSNL.getLength(); nPos--; )
 				xNewKey->createKey( pArray[nPos] );

 			return sal_True;
 		}
 		catch (InvalidRegistryException &)
 		{
 			OSL_ENSURE( sal_False, "### InvalidRegistryException!" );
 		}
 	}

 	return sal_False;
 }

/**
 * This function is called to get service factories for an implementation.
 *
 * @param pImplName       name of implementation
 * @param pServiceManager a service manager, need for component creation
 * @param pRegistryKey    the registry key for this component, need for persistent data
 * @return a component factory
 */
extern "C" void * SAL_CALL component_getFactory(const sal_Char * pImplName,
                                                void *           pServiceManager,
                                                void *           pRegistryKey)
{
    if ( !pServiceManager || !pImplName )
        return 0;

    printf ("component_getFactory %s\n", IMPLEMENTATION_NAME);
    if (rtl_str_compare( pImplName, IMPLEMENTATION_NAME ) == 0)
    {
        Reference< XSingleComponentFactory > xFactory =
            ::cppu::createSingleComponentFactory(
                Addon_createInstance,
                OUString::createFromAscii( IMPLEMENTATION_NAME ),
                Addon_getSupportedServiceNames(),
                NULL);

        if (xFactory.is())
        {
            printf ("component_getFactory xFactory.is()\n");
            xFactory->acquire();
            return xFactory.get();
        }
    }

    return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
