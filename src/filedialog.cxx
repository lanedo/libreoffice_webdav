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

#include "filedialog.hxx"
#include <osl/diagnose.h>
#include <rtl/ustring.hxx>
#include <cppuhelper/implbase1.hxx>
#include <com/sun/star/awt/Key.hpp>
#include <com/sun/star/awt/PosSize.hpp>
#include <com/sun/star/awt/WindowAttribute.hpp>
#include <com/sun/star/awt/XButton.hpp>
#include <com/sun/star/awt/XControl.hpp>
#include <com/sun/star/awt/XControlContainer.hpp>
#include <com/sun/star/awt/XControlModel.hpp>
#include <com/sun/star/awt/XDialog.hpp>
#include <com/sun/star/awt/XDialogProvider2.hpp>
#include <com/sun/star/awt/XWindowPeer.hpp>
#include <com/sun/star/awt/XMessageBox.hpp>
#include <com/sun/star/awt/MessageBoxButtons.hpp>
#include <com/sun/star/awt/XMessageBoxFactory.hpp>
#include <com/sun/star/awt/XItemList.hpp>
#include <com/sun/star/awt/XListBox.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/deployment/PackageInformationProvider.hpp>
#include <com/sun/star/deployment/XPackageInformationProvider.hpp>
#include <com/sun/star/frame/FrameSearchFlag.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/frame/XUntitledNumbers.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/ucb/XSimpleFileAccess.hpp>

#ifdef DEBUG
    #include <cstdio>
#else
    #define printf(...)
#endif

using rtl::OUString;
using namespace css::awt;
using namespace css::beans;
using namespace css::container;
using namespace css::deployment;
using namespace css::frame;
using namespace css::lang;
using namespace css::uno;
using css::lang::XMultiComponentFactory;
using css::awt::Key::RETURN;
using css::awt::PosSize::POSSIZE;

namespace WebDAVUI {

/* Action listener */
class FileDialogActionListener : public ::cppu::WeakImplHelper1< css::awt::XActionListener >
{
private:
    FileDialog * const owner;

public:
    FileDialogActionListener (FileDialog * const _owner)
       : owner (_owner) { }

    // XEventListener
    virtual void SAL_CALL disposing (const css::lang::EventObject &aEventObj) throw (css::uno::RuntimeException)
    {
        printf ("FileDialogActionListener::disposing\n");
    }

    // XActionListener
    virtual void SAL_CALL actionPerformed (const css::awt::ActionEvent &rEvent) throw (css::uno::RuntimeException)
    {
        /* Obtain the name of the control the event originated from */
        Reference< XControl > control (rEvent.Source, UNO_QUERY);
        Reference< XControlModel > controlModel = control->getModel ();
        Reference< XPropertySet > controlProps (controlModel, UNO_QUERY);
        css::uno::Any aValue = controlProps->getPropertyValue (OUString::createFromAscii ("Name"));
        OUString controlName;
        aValue >>= controlName;
        printf ("FileDialog::actionPerformed %s\n",
                OUStringToOString (controlName, RTL_TEXTENCODING_UTF8).getStr ());

        if (controlName.equalsAscii ("OpenButton")
         || controlName.equalsAscii ("SaveButton"))
        {
            owner->openOrSaveSelectedDocument ();
        }
        else if (controlName.equalsAscii ("FileList"))
        {
            /* This will work fine, because the first click will
             * select the item.  Which means the item will be
             * selected when we enter this method.
             */
            owner->openOrSaveSelectedDocument ();
        }
        else if (controlName.equalsAscii ("OpenLocationButton"))
        {
            /* FIXME: Is this okay with regard to threading, etc. ? */
            owner->listFiles ();
        }
        else if (controlName.equalsAscii ("CancelButton"))
        {
            owner->closeDialog ();
        }
    }
};

/* Item listener */
class FileDialogItemListener : public ::cppu::WeakImplHelper1< css::awt::XItemListener >
{
private:
    FileDialog * const owner;

public:
    FileDialogItemListener (FileDialog * const _owner)
       : owner (_owner) { }

    // XEventListener
    virtual void SAL_CALL disposing (const css::lang::EventObject &aEventObj) throw (css::uno::RuntimeException)
    {
        printf ("FileDialogActionListener::disposing\n");
    }

    // XItemListener
    virtual void SAL_CALL itemStateChanged (const css::awt::ItemEvent &rEvent) throw (css::uno::RuntimeException)
    {
        /* Obtain the name of the control the event originated from */
        Reference< XControl > control (rEvent.Source, UNO_QUERY);
        Reference< XControlModel > controlModel = control->getModel ();
        Reference< XPropertySet > controlProps (controlModel, UNO_QUERY);
        css::uno::Any aValue = controlProps->getPropertyValue (OUString::createFromAscii ("Name"));
        OUString controlName;
        aValue >>= controlName;
        printf ("FileDialog::itemStateChanged %s\n",
                OUStringToOString (controlName, RTL_TEXTENCODING_UTF8).getStr ());

        if (controlName.equalsAscii ("FileList"))
        {
            Reference< XListBox > listBox (control, UNO_QUERY);

            printf ("Selected item: %ld\n", rEvent.Selected);

            if (rEvent.Selected >= 0 &&
                rEvent.Selected < listBox->getItemCount ())
            {
                OUString itemString = listBox->getItem (rEvent.Selected);

                printf ("Selected item: %ld (%s)\n",
                        rEvent.Selected,
                        OUStringToOString (itemString, RTL_TEXTENCODING_UTF8).getStr ());

                owner->setFilename (itemString);
            }
        }
    }
};

/* Key listener */
class FileDialogKeyListener : public ::cppu::WeakImplHelper1< css::awt::XKeyListener >
{
private:
    FileDialog * const owner;

public:
    FileDialogKeyListener (FileDialog * const _owner)
       : owner (_owner) { }

    // XEventListener
    virtual void SAL_CALL disposing (const css::lang::EventObject &aEventObj) throw (css::uno::RuntimeException)
    {
        printf ("FileDialogKeyListener::disposing");
    }

    // XKeyListener
    virtual void SAL_CALL keyPressed (const css::awt::KeyEvent &rEvent) throw (css::uno::RuntimeException)
    {
        /* Obtain the name of the control the event originated from */
        Reference< XControl > control (rEvent.Source, UNO_QUERY);
        Reference< XControlModel > controlModel = control->getModel ();
        Reference< XPropertySet > controlProps (controlModel, UNO_QUERY);
        css::uno::Any aValue = controlProps->getPropertyValue (OUString::createFromAscii ("Name"));
        OUString controlName;
        aValue >>= controlName;

        printf ("FileDialogKeyListener::keyPressed: %s\n",
                OUStringToOString (controlName, RTL_TEXTENCODING_UTF8).getStr ());

        if (controlName.equalsAscii ("LocationEntry"))
        {
            short aKey = rEvent.KeyCode;

            if (aKey == RETURN)
            {
                owner->listFiles ();
            }
        }
        else if (controlName.equalsAscii ("FileList") ||
                 controlName.equalsAscii ("FileEntry"))
        {
            short aKey = rEvent.KeyCode;

            if (aKey == RETURN)
            {
                owner->openOrSaveSelectedDocument ();
            }
        }
    }

    virtual void SAL_CALL keyReleased (const css::awt::KeyEvent &rEvent) throw (css::uno::RuntimeException)
    {
        printf ("FileDialogKeyListener::keyReleased\n");
    }
};


/* Dialog construction */

FileDialog::FileDialog( const Reference< css::uno::XComponentContext > &rxContext,
                        const Reference< css::frame::XFrame >          &rxFrame,
                        WebDAVUI::Settings*                             rSettings,
                        const sal_Bool                                  isSave) : mxContext ( rxContext ),
                                                                                  mxFrame ( rxFrame ),
                                                                                  mSettings ( rSettings),
                                                                                  isSave ( isSave)
{
    printf ("FileDialog::FileDialog\n");

    mxMCF = mxContext->getServiceManager ();

    // Create the toolkit to have access to it later
    mxToolkit = Reference< XToolkit >( mxMCF->createInstanceWithContext(
                                        OUString( RTL_CONSTASCII_USTRINGPARAM(
                                            "com.sun.star.awt.Toolkit" )), mxContext), UNO_QUERY );

    createDialog ();
    listFiles ();
}

static OUString extensionForStorable (Reference< XStorable > document)
{
    Reference< XServiceInfo > xServiceInfo (document, UNO_QUERY_THROW);
    if (xServiceInfo->supportsService (OUString::createFromAscii ("com.sun.star.text.TextDocument")))
        return OUString::createFromAscii (".odt");
    else if (xServiceInfo->supportsService (OUString::createFromAscii ("com.sun.star.drawing.DrawingDocument")))
        return OUString::createFromAscii (".odg");
    else if (xServiceInfo->supportsService (OUString::createFromAscii ("com.sun.star.formula.FormulaProperties")))
        return OUString::createFromAscii (".odf");
    else if (xServiceInfo->supportsService (OUString::createFromAscii ("com.sun.star.presentation.PresentationDocument")))
        return OUString::createFromAscii (".odp");
    else if (xServiceInfo->supportsService (OUString::createFromAscii ("com.sun.star.sheet.SpreadsheetDocument")))
        return OUString::createFromAscii (".ods");
    else if (xServiceInfo->supportsService (OUString::createFromAscii ("com.sun.star.text.WebDocument")))
        return OUString::createFromAscii (".html");
    return OUString::createFromAscii (".UNKNOWN");
}

void FileDialog::createDialog (void)
{
    dialog = mSettings->createDialog (OUString::createFromAscii ("OpenDialog"));
    Reference< XDialog > realDialog (dialog, UNO_QUERY);

    if (isSaveDialog ())
    {
        realDialog->setTitle (mSettings->localizedString (LocalizedStrings::windowTitleSave));
    }
    else
    {
        realDialog->setTitle (mSettings->localizedString (LocalizedStrings::windowTitleOpen));
    }

    /* Put the dialog in a window */
    Reference< XControl > control (dialog, UNO_QUERY);
    Reference< XWindow > window (control, UNO_QUERY);
    window->setVisible (true);
    control->createPeer (mxToolkit, NULL);

    /* The dialog is an XControlContainer */
    Reference< XControlContainer > controlContainer (dialog, UNO_QUERY);

    /* Get the logo image */
    Reference< XControl > logoImage =
        controlContainer->getControl (OUString::createFromAscii ("LogoImage"));
    Reference< XControlModel > logoImageModel = logoImage->getModel ();
    Reference< XPropertySet > imageProps (logoImageModel, UNO_QUERY);
    imageProps->setPropertyValue (OUString::createFromAscii ("ImageURL"),
                                  makeAny (mSettings->getImageURL (ImageKeys::imageURLLogo)));

    /* Get the open/save button */
    openButton = controlContainer->getControl (OUString::createFromAscii ("OpenButton"));
    Reference< XControlModel > openButtonModel = openButton->getModel ();
    Reference< XWindow > openButtonWindow (openButton, UNO_QUERY);
    openButtonWindow->setVisible (!isSaveDialog ());

    saveButton = controlContainer->getControl (OUString::createFromAscii ("SaveButton"));
    Reference< XControlModel > saveButtonModel = saveButton->getModel ();
    Reference< XWindow > saveButtonWindow (saveButton, UNO_QUERY);
    saveButtonWindow->setVisible (isSaveDialog ());

    /* Read-only checkbox */
    Reference< XControl > roCheckBox =
        controlContainer->getControl (OUString::createFromAscii ("ReadOnlyCheckBox"));
    Reference< XWindow > roCheckBoxWindow (roCheckBox, UNO_QUERY);
    roCheckBoxWindow->setVisible (false); //!isSaveDialog ();

    /* Create event listeners */
    Reference< XActionListener > actionListener =
        static_cast< XActionListener *> (new FileDialogActionListener (this));

    Reference< XItemListener > itemListener =
        static_cast< XItemListener *> (new FileDialogItemListener (this));

    Reference< XKeyListener > keyListener =
        static_cast< XKeyListener *> (new FileDialogKeyListener (this));

    Reference< XButton > openButtonControl (openButton, UNO_QUERY);
    openButtonControl->addActionListener (actionListener);
    Reference< XButton > saveButtonControl (saveButton, UNO_QUERY);
    saveButtonControl->addActionListener (actionListener);

    /* Connect List URL button to action listener */
    Reference< XInterface > buttonObject =
        controlContainer->getControl (OUString::createFromAscii ("OpenLocationButton"));
    Reference< XButton > buttonControl (buttonObject, UNO_QUERY);
    buttonControl->addActionListener (actionListener);

    /* hide the open location button */
    Reference< XWindow > locationButtonWindow (buttonControl, UNO_QUERY);
    locationButtonWindow->setVisible (false);

    /* Connect cancel button to action listener */
    Reference< XInterface > cancelObject =
        controlContainer->getControl (OUString::createFromAscii ("CancelButton"));
    Reference< XButton > cancelControl (cancelObject, UNO_QUERY);

    cancelControl->addActionListener (actionListener);

    Reference< XControl > labelControl =
        controlContainer->getControl (OUString::createFromAscii ("FileEntryLabel"));
    Reference< XWindow > labelControlWindow (labelControl, UNO_QUERY);
    labelControlWindow->setVisible (isSaveDialog ());

    Reference< XControl > entryControl =
        controlContainer->getControl (OUString::createFromAscii ("FileEntry"));
    Reference< XWindow > entryControlWindow (entryControl, UNO_QUERY);
    entryControlWindow->setVisible (isSaveDialog ());

    fileEntryModel = entryControl->getModel ();
    Reference< XPropertySet > entryProps (fileEntryModel, UNO_QUERY);
    if (isSaveDialog ())
    {
        Reference< XController > xController = mxFrame->getController();
        if (!xController.is())
            return;
        Reference< XModel > xModel (xController->getModel ());
        Reference< XStorable > xStorable (xModel, UNO_QUERY);
        if (!xStorable.is ())
            return;

        OUString fileName;
        if (xStorable->hasLocation ())
        {
            fileName = xStorable->getLocation().copy (
                xStorable->getLocation().lastIndexOf (OUString::createFromAscii ("/")) + 1);
        }
        else
        {
            Reference< css::frame::XUntitledNumbers > untitledNumbers (
                mxMCF->createInstanceWithContext (
                    OUString::createFromAscii ("com.sun.star.frame.Desktop"),
                    mxContext), UNO_QUERY);
            fileName = untitledNumbers->getUntitledPrefix () + extensionForStorable (xStorable);
        }
        entryProps->setPropertyValue (OUString::createFromAscii ("Text"), makeAny (fileName));
    }

    /* Save references to the file list and location entry models */
    Reference< XControl > listControl =
        controlContainer->getControl (OUString::createFromAscii ("FileList"));
    fileListModel = listControl->getModel ();

    /* Make the file list higher in the open dialog */
    if (!isSaveDialog ())
    {
        Reference< XWindow > listControlWindow (listControl, UNO_QUERY);

        Rectangle rect = listControlWindow->getPosSize ();
        listControlWindow->setPosSize (rect.X, rect.Y, rect.Width, rect.Height + 60, POSSIZE);
    }

    OUString remoteServer (mSettings->getRemoteServerName ());

    /* Connect the entry to a key listener and get its model */
    entryControl =
        controlContainer->getControl (OUString::createFromAscii ("LocationEntry"));
    locationEntryModel = entryControl->getModel ();
    entryProps = Reference< XPropertySet> (locationEntryModel, UNO_QUERY);
    entryProps->setPropertyValue (OUString::createFromAscii ("Text"),
                                  makeAny (remoteServer));

    /* hide the location entry */
    Reference< XWindow > locationEntryWindow (entryControl, UNO_QUERY);
    locationEntryWindow->setVisible (false);

    Reference< XControl > locationLabelControl =
        controlContainer->getControl (OUString::createFromAscii ("Location"));
    Reference< XInterface > locationLabelModel (locationLabelControl->getModel ());
    Reference< XPropertySet > labelProps (locationLabelModel, UNO_QUERY);
    labelProps->setPropertyValue (OUString::createFromAscii ("Label"),
                                  makeAny (remoteServer));

    Reference< XWindow > entryWindow (entryControl, UNO_QUERY);
    entryWindow->addKeyListener (keyListener);

    /* hide the file type combo */
    Reference< XControl > typeLabelControl =
        controlContainer->getControl (OUString::createFromAscii ("TypeLabel"));
    Reference< XWindow > typeLabelWindow (typeLabelControl, UNO_QUERY);
    typeLabelWindow->setVisible (false);

    Reference< XControl > typeEntryControl =
        controlContainer->getControl (OUString::createFromAscii ("TypeEntry"));
    Reference< XWindow > typeEntryWindow (typeEntryControl, UNO_QUERY);
    typeEntryWindow->setVisible (false);

    /* Connect the list box to an action listener */
    Reference< XListBox > listBox (listControl, UNO_QUERY);
    listBox->addActionListener (actionListener);
    listBox->addItemListener (itemListener);
    Reference< XWindow > listWindow (listControl, UNO_QUERY);
    listWindow->addKeyListener (keyListener);

    /* Finally, set the focus to the file list */
    listWindow->setFocus ();
}

sal_Bool FileDialog::isSaveDialog (void)
{
    return isSave;
}

void FileDialog::show (void)
{
    /* Execute the clear */
    Reference< XDialog > xDialog(dialog,UNO_QUERY);
    xDialog->execute();

    /* After execution: get the XComponent interface of the dialog
     * and dispose the dialog.
     */
    Reference< XComponent > xComponent(dialog,UNO_QUERY);
    xComponent->dispose();
}

void FileDialog::closeDialog (void)
{
    Reference< XDialog > xDialog(dialog,UNO_QUERY);
    xDialog->endExecute();
}

short FileDialog::showMessageBox (OUString errorMessage, bool confirm)
{
    if (!(mxFrame.is () && mxToolkit.is ()))
        return 0;

    Reference< XMessageBoxFactory > xMessageBoxFactory( mxToolkit, UNO_QUERY );
    Rectangle aRectangle;
    OUString aType;
    long aButtons;
    OUString aTitle = mSettings->localizedString (LocalizedStrings::windowTitleError);
    if (confirm)
    {
        aType = OUString (RTL_CONSTASCII_USTRINGPARAM ("querybox"));
        aButtons = MessageBoxButtons::BUTTONS_OK_CANCEL;
    }
    else
    {
        aType = OUString (RTL_CONSTASCII_USTRINGPARAM ("infobox"));
        aButtons = MessageBoxButtons::BUTTONS_OK;
    }

    Reference< XWindow > xContainerWindow (mxFrame->getContainerWindow ());
    Reference< XWindowPeer > xWindowPeer (xContainerWindow, UNO_QUERY_THROW);
    Reference< XMessageBox > xMessageBox = xMessageBoxFactory->createMessageBox (
        xWindowPeer, aRectangle, aType, aButtons, aTitle, errorMessage);

    if (xMessageBox.is ())
    {
        short returnValue = xMessageBox->execute ();
        /* 0 = Cancel, 1 = OK */
        return returnValue;
    }

    return 0;
}

void FileDialog::openOrSaveSelectedDocument (void)
{
    if ( ! (mxFrame.is() && mxToolkit.is()) )
        return;

    Reference< XComponentLoader > xLoader(mxFrame, UNO_QUERY);
    Sequence< PropertyValue > lProperties (1);
    /* Set interaction handler */
    Reference< css::task::XInteractionHandler > interactionHandler =
        Reference< css::task::XInteractionHandler > (
            mxMCF->createInstanceWithContext (
            OUString::createFromAscii ("com.sun.star.task.InteractionHandler"),
            mxContext), UNO_QUERY);
    lProperties[0].Name = OUString::createFromAscii ("InteractionHandler");
    lProperties[0].Value = makeAny (interactionHandler);

    if (isSaveDialog ())
    {
        Reference< XPropertySet > entryProps (fileEntryModel, UNO_QUERY);
        Any aValue = entryProps->getPropertyValue (OUString::createFromAscii ("Text"));
        OUString sURL;
        aValue >>= sURL;
        sURL = mSettings->getRemoteServerName () + sURL;

        printf ("Saving document: %s\n",
                OUStringToOString (sURL, RTL_TEXTENCODING_UTF8).getStr ());

        if (sURL.equals (mSettings->getRemoteServerName ()))
        {
            showMessageBox (mSettings->localizedString (LocalizedStrings::emptyFilename), false);
            return;
        }

        {
            const Reference< XItemList > items(fileListModel, UNO_QUERY_THROW );
            sal_Int32 i = 0;
            while (true)
            {
                OUString sItemURL;
                try {
                    Any aItemURL = items->getItemData(i);
                    aItemURL >>= sItemURL;
                    i++;
                }
                catch (...)
                {
                    /* No items left raises an exception */
                    break;
                }
                printf ("Existing... %s\n",
                    OUStringToOString (sItemURL, RTL_TEXTENCODING_UTF8).getStr ());
                if (sItemURL.equals (sURL))
                {
                    short returnValue = showMessageBox (mSettings->localizedString (
                        LocalizedStrings::existingFilename), true);
                    /* 0 = Cancel, 1 = OK */
                    if (returnValue == 0)
                        return;
                    break;
                }
            }
        }

        Reference< XController > xController = mxFrame->getController ();
        if (!xController.is ())
            return;
        Reference< XModel > xModel (xController->getModel ());
        Reference< XStorable > xStorable(xModel, UNO_QUERY);
        if (!xStorable.is ())
            return;

        try
        {
            xStorable->storeAsURL(sURL, lProperties);
        }
        catch (Exception & e)
        {
            printf ("Failed to save: %s\n",
                    OUStringToOString (e.Message, RTL_TEXTENCODING_ASCII_US).getStr ());
        }

        /* Close dialog when done */
        closeDialog ();
        return;
    }

    Reference< XPropertySet > entryProps (fileListModel, UNO_QUERY);
    Any aValue = entryProps->getPropertyValue (OUString::createFromAscii ("SelectedItems"));
    Sequence< short > selectedItems;
    aValue >>= selectedItems;

    sal_Int32 n = selectedItems.getLength ();
    for (sal_Int32 i = 0; i < n; i++)
    {
        const Reference< XItemList > items(fileListModel, UNO_QUERY_THROW );
        Any aURL = items->getItemData(selectedItems[i]);
        OUString sURL;
        aURL >>= sURL;
        /* No URL, so this was informational or an error message */
        if ( sURL.getLength() == 0 )
            return;

        try
        {
            printf ("Opening document: %s\n",
                    OUStringToOString (sURL, RTL_TEXTENCODING_UTF8).getStr ());
            Reference< XComponent > xDocument (xLoader->loadComponentFromURL(
                sURL, mxFrame->getName(), FrameSearchFlag::CHILDREN, lProperties));
        }
        catch (Exception & e)
        {
            printf ("Failed to open: %s\n",
                    OUStringToOString (e.Message, RTL_TEXTENCODING_ASCII_US).getStr ());
        }
    }

    /* Close dialog when done */
    closeDialog ();
}

void FileDialog::listFiles (void)
{
    /* Get text from our location entry */
    Reference< XPropertySet > entryProps (locationEntryModel, UNO_QUERY);
    css::uno::Any aValue = entryProps->getPropertyValue (OUString::createFromAscii ("Text"));
    OUString url;
    aValue >>= url;

    printf ("Accessing WebDAV server, trying location %s ...\n",
            OUStringToOString (url, RTL_TEXTENCODING_UTF8).getStr ());

    /* Create a reference to the SimpleFileAccess service */
    Reference< css::ucb::XSimpleFileAccess > fileAccess =
        Reference< css::ucb::XSimpleFileAccess > (
                mxMCF->createInstanceWithContext (OUString::createFromAscii ("com.sun.star.ucb.SimpleFileAccess"), mxContext), UNO_QUERY);

    if (!fileAccess.is ())
    {
        printf ("Could not create SimpleFileAccess object\n");
        return;
    }

    /* Set up an interaction handler, which will handle e.g. requesting
     * credentials from the user.
     */
    Reference< css::task::XInteractionHandler > interactionHandler =
        Reference< css::task::XInteractionHandler > (
                mxMCF->createInstanceWithContext (OUString::createFromAscii ("com.sun.star.task.InteractionHandler"), mxContext), UNO_QUERY);
    fileAccess->setInteractionHandler (interactionHandler);
    const Reference< XItemList > items(fileListModel, UNO_QUERY_THROW );
    items->removeAllItems();
    bool canOpen = false;
    bool canSave = true;

    /* Now try to access the folder */
    try
    {
        Sequence< rtl::OUString > entries = fileAccess->getFolderContents (url, false);
        const OUString *stringArray = entries.getConstArray ();
        sal_Int32 n = entries.getLength ();

        for (sal_Int32 i = 0; i < n; i++)
        {
            OUString fileName (stringArray[i].copy (
                stringArray[i].lastIndexOf (OUString::createFromAscii ("/")) + 1));
            items->insertItem(0, fileName,
                OUString::createFromAscii (""));
            items->setItemData(0, makeAny (stringArray[i]));
        }
        canOpen = n > 0;
    }
    catch ( ... ) /* FIXME: Need proper exception handling here */
    {
        items->insertItemText (0, mSettings->localizedString (LocalizedStrings::contentListFailure));
        canSave = false;
    }

    Reference< XControlModel > openButtonModel = openButton->getModel ();
    Reference< XWindow > openButtonWindow (openButton, UNO_QUERY);
    openButtonWindow->setEnable (canOpen);
    Reference< XControlModel > saveButtonModel = saveButton->getModel ();
    Reference< XWindow > saveButtonWindow (saveButton, UNO_QUERY);
    saveButtonWindow->setEnable (canSave);
}

void FileDialog::setFilename (OUString filename)
{
    Reference< XPropertySet > entryProps (fileEntryModel, UNO_QUERY);
    entryProps->setPropertyValue (OUString::createFromAscii ("Text"),
                                  makeAny (filename));
}

}
