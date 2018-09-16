
#include <KCoreAddons/KPluginFactory>
#include <KFileItemListProperties>
#include <KFileItem>
#include <KLocalizedString>
#include <KToolInvocation>
#include <QAction>
#include <QMenu>
#include <QFileInfo>
#include <QStringList>
#include "bcompare_ext_kde5.h"


/*************************************************************
 * Utilities
 *************************************************************/

bool BCompareKde5::isPathConsideredFolder(const QString &path) const
{
    if (!path.isEmpty() && (QFileInfo(path).isDir() || m_config.isFileArchive(path)))
    {
        return true;
    }
    return false;
}

void BCompareKde5::clearSelections()
{
    m_config.forgetLeftFile();
    m_config.forgetCenterFile();
}

/*************************************************************
 * Action callbacks
 *************************************************************/

void BCompareKde5::cbSelectLeft()
{
    m_config.savePathLeftFile(m_pathRightFile);
}

void BCompareKde5::cbSelectCenter()
{
    m_config.savePathCenterFile(m_pathRightFile);
}

void BCompareKde5::cbEditFile()
{
    KToolInvocation::kdeinitExec(QLatin1String("bcompare"),
                                 QStringList{ QLatin1String("-edit"), m_pathRightFile });
    clearSelections();
}

void BCompareKde5::cbCompare()
{
    QAction* srcAction = qobject_cast<QAction*>(sender());

    if (srcAction != nullptr)
    {
        QStringList args{ m_pathLeftFile, m_pathRightFile };
        QString viewer = srcAction->data().toString();

        if (!viewer.isEmpty())
        {
            args.prepend(QString(QLatin1String("-fv=%1")).arg(viewer));
        }

        KToolInvocation::kdeinitExec(QLatin1String("bcompare"), args);
        clearSelections();
    }
}

void BCompareKde5::cbSync()
{
    KToolInvocation::kdeinitExec(QLatin1String("bcompare"),
                                 QStringList{ m_pathLeftFile, m_pathRightFile });
    clearSelections();
}

void BCompareKde5::cbMerge()
{
    QStringList args{ QLatin1String("-fv=Text Merge"), m_pathLeftFile, m_pathRightFile };
    if (!m_pathCenterFile.isEmpty())
    {
        args.append(m_pathCenterFile);
    }
    KToolInvocation::kdeinitExec(QLatin1String("bcompare"), args);
    clearSelections();
}

/*************************************************************
 * Menu Items
 *************************************************************/

QAction *BCompareKde5::createMenuItem(const QString &txt, const QString &hint, bool isIconHalf,
                                      void (BCompareKde5::*callback)())
{
    QAction *item = new QAction(this);
    item->setText(txt);
    item->setToolTip(hint);
    item->setIcon(isIconHalf ? m_config.menuIconHalf() : m_config.menuIconFull());

    connect(item, &QAction::triggered, this, callback);

    return item;
}

QAction *BCompareKde5::createMenuItemSelectLeft(const CreateMenuCtx &ctx)
{
    QStringList nextActions;

    if (ctx.nbSelected == 1)
    {
        if ((m_config.menuCompare() == ctx.menuType) ||
            (m_config.menuCompareUsing() == ctx.menuType && !ctx.isDir))
        {
            nextActions.append(i18nc("@bc compare action", "Compare"));
        }

        if (m_config.menuMerge() == ctx.menuType)
        {
            nextActions.append(i18nc("@bc merge action", "Merge"));
        }

        if (m_config.menuSync() == ctx.menuType)
        {
            nextActions.append(i18nc("@bc sync action", "Sync"));
        }
    }

    if (nextActions.size() > 0)
    {
        QString itemStr = ctx.isDir ? i18nc("@bc selected folder type", "Folder") :
                                      i18nc("@bc selected file type", "File");

        QString menuStr = i18n("Select Left %1 for %2", itemStr, nextActions.join(QLatin1Char('/')));

        QString hintStr = i18n("Remembers selected item for later comparison using Beyond Compare. "
                               "Right-click another item to start the comparison");

        return createMenuItem(menuStr, hintStr, true, &BCompareKde5::cbSelectLeft);
    }

    return nullptr;
}

QAction *BCompareKde5::createMenuItemSelectCenter(const CreateMenuCtx &ctx)
{
    if (m_config.menuMerge() == ctx.menuType && ctx.nbSelected == 1)
    {
        return createMenuItem(i18n("Select Center File"),
                              i18n("Remembers selected item for later comparison using Beyond Compare. "
                                   "Right-click another item to start the comparison"),
                              true, &BCompareKde5::cbSelectCenter);
    }
    return nullptr;
}

QAction *BCompareKde5::createMenuItemEdit(const CreateMenuCtx &ctx)
{
    if (m_config.menuEdit() == ctx.menuType && ctx.nbSelected == 1 && !ctx.isDir)
    {
        QString menuStr = (ctx.menuType == BCompareConfig::MENU_SUBMENU) ?
                           i18nc("@bc edit menu", "Edit") : i18n("Edit with Beyond Compare");

        return createMenuItem(menuStr, i18n("Edit the file using Beyond Compare"),
                              false, &BCompareKde5::cbEditFile);
    }
    return nullptr;
}

QAction *BCompareKde5::createMenuItemCompare(const CreateMenuCtx &ctx)
{
    QString menuStr;
    QString hintStr;

    if (m_config.menuCompare() == ctx.menuType)
    {
        if (ctx.nbSelected == 1 && !m_pathLeftFile.isEmpty())
        {
            menuStr = i18nc("@bc compare to menu", "Compare to '%1'", QFileInfo(m_pathLeftFile).fileName());
            hintStr = i18n("Compare selected item with previously selected left item, using Beyond Compare");
        }
        else if (ctx.nbSelected == 2)
        {
            menuStr = i18nc("@bc compare menu", "Compare");
            hintStr = i18n("Compare selected items using Beyond Compare");
        }
    }

    if (!menuStr.isEmpty())
    {
        return createMenuItem(menuStr, hintStr, false, &BCompareKde5::cbCompare);
    }
    return nullptr;
}

QAction *BCompareKde5::createMenuItemCompareUsing(const QString &fileViewer,
                                                  const CreateMenuCtx &ctx)
{
    QString menuStr;
    QString hintStr;

    if (m_config.menuCompareUsing() == ctx.menuType && !ctx.isDir)
    {
        if (ctx.nbSelected == 1 && !m_pathLeftFile.isEmpty())
        {
            menuStr = i18nc("@bc compare using to menu", "%1 to '%2'",
                            fileViewer, QFileInfo(m_pathLeftFile).fileName());

            hintStr = i18n("Compare selected item with previously selected left item, "
                           "using Beyond Compare %1", fileViewer);
        }
        else if (ctx.nbSelected == 2)
        {
            menuStr = i18n("Compare files using the '%1' viewer", fileViewer);
            hintStr = i18n("Compare selected items using Beyond Compare");
        }
    }

    if (!menuStr.isEmpty())
    {
        QAction *act = createMenuItem(menuStr, hintStr, false, &BCompareKde5::cbCompare);
        act->setData(fileViewer);
        return act;
    }
    return nullptr;
}

QAction *BCompareKde5::createMenuItemSync(const CreateMenuCtx &ctx)
{
    QString menuStr;
    QString hintStr;

    if (m_config.menuSync() == ctx.menuType && ctx.isDir)
    {
        if (ctx.nbSelected == 1 && !m_pathLeftFile.isEmpty())
        {
            menuStr = i18nc("@bc sync with menu", "Sync with '%1'", QFileInfo(m_pathLeftFile).fileName());
            hintStr = i18n("Sync to previously selected folder");
        }
        else if (ctx.nbSelected == 2)
        {
            menuStr = i18nc("@bc sync menu", "Sync");
            hintStr = i18n("Sync two selected folders");
        }
    }

    if (!menuStr.isEmpty())
    {
        return createMenuItem(menuStr, hintStr, false, &BCompareKde5::cbSync);
    }
    return nullptr;
}

QAction *BCompareKde5::createMenuItemMerge(const CreateMenuCtx &ctx)
{
    QString menuStr;
    QString hintStr;

    if (m_config.menuMerge() == ctx.menuType)
    {
        if (ctx.nbSelected == 1 && !m_pathLeftFile.isEmpty() && !m_pathCenterFile.isEmpty())
        {
            menuStr = i18nc("@bc merge with left center menu", "Merge with '%1', '%2'",
                            QFileInfo(m_pathLeftFile).fileName(),
                            QFileInfo(m_pathCenterFile).fileName());

            hintStr = i18n("Merge file with previously selected left and center files using Beyond Compare");
        }
        if (ctx.nbSelected == 1 && !m_pathLeftFile.isEmpty())
        {
            menuStr = i18nc("@bc merge with left menu", "Merge with '%1'",
                            QFileInfo(m_pathLeftFile).fileName());

            hintStr = i18n("Merge file with previously selected left file using Beyond Compare");
        }
        else if (ctx.nbSelected == 2 && !m_pathCenterFile.isEmpty())
        {
            menuStr = i18nc("@bc merge with center menu", "Merge with '%1'",
                            QFileInfo(m_pathCenterFile).fileName());

            hintStr = i18n("Merge selected files (left, right) with previously selected center file");
        }
        else if (ctx.nbSelected == 2)
        {
            menuStr = i18nc("@bc merge menu", "Merge");
            hintStr = i18n("Merge selected files (left, right)");
        }
        else if (ctx.nbSelected == 3)
        {
            menuStr = i18nc("@bc merge menu", "Merge");
            hintStr = i18n("Merge selected files (left, right, center)");
        }
    }

    if (!menuStr.isEmpty())
    {
        return createMenuItem(menuStr, hintStr, false, &BCompareKde5::cbMerge);
    }
    return nullptr;
}

/*************************************************************
 * Menu Item creation
 *************************************************************/

static void addItemToListIfNonNull(QList<QAction*> &items, QAction *item)
{
    if (item != nullptr)
    {
        items.append(item);
    }
}

void BCompareKde5::createMenus(QList<QAction*> &items, const CreateMenuCtx &ctx)
{
    addItemToListIfNonNull(items, createMenuItemMerge(ctx));
    addItemToListIfNonNull(items, createMenuItemCompare(ctx));

    if (m_config.menuCompareUsing() == ctx.menuType && !ctx.isDir)
    {
        bool listEmpty = true;
        QMenu *subMenu = new QMenu();
        QAction *subMenuAction = new QAction(this);

        subMenuAction->setMenu(subMenu);
        subMenu->setTitle(i18n("Compare Using"));
        subMenu->setIcon(m_config.menuIconFull());

        QStringList::const_iterator itEnd = m_config.listViewer().constEnd();
        for (QStringList::const_iterator it = m_config.listViewer().constBegin(); it != itEnd; ++it)
        {
            QAction *act = createMenuItemCompareUsing(*it, ctx);
            if (act != nullptr)
            {
                listEmpty = false;
                subMenu->addAction(act);
            }
        }

        if (listEmpty)
        {
            delete subMenuAction;
        }
        else
        {
            items.append(subMenuAction);
        }
    }

    addItemToListIfNonNull(items, createMenuItemSync(ctx));
    addItemToListIfNonNull(items, createMenuItemSelectLeft(ctx));
    addItemToListIfNonNull(items, createMenuItemSelectCenter(ctx));
    addItemToListIfNonNull(items, createMenuItemEdit(ctx));
}

/*************************************************************
 * Entry point of this plugin
 *************************************************************/

BCompareKde5::BCompareKde5(QObject *pParent, const QVariantList &) :
    KAbstractFileItemActionPlugin(pParent), m_config(BCompareConfig::get())
{
}

BCompareKde5::~BCompareKde5()
{
}

QList<QAction*> BCompareKde5::actions(const KFileItemListProperties &fileItemInfos, QWidget *)
{
    QList<QAction*> listActions;
    KFileItemList selectedFiles = fileItemInfos.items();
    int nbSelected = selectedFiles.size();

    /* Do not handle more than 3 selected items */
    if (nbSelected <= 0 || nbSelected > 3 || !m_config.menuEnabled())
    {
        return listActions;
    }

    /* All the selected items must be considered of the same type */
    bool firstIsDir = isPathConsideredFolder(selectedFiles.at(0).url().path());
    for (int i = 1; i < selectedFiles.size(); ++i)
    {
        if (firstIsDir != isPathConsideredFolder(selectedFiles.at(i).url().path()))
        {
            return listActions;
        }
    }

    if (nbSelected == 3)
    {
        m_pathLeftFile = selectedFiles.at(0).url().path();
        m_pathRightFile = selectedFiles.at(1).url().path();
        m_pathCenterFile = selectedFiles.at(2).url().path();
    }
    else if (nbSelected == 2)
    {
        m_pathLeftFile = selectedFiles.at(0).url().path();
        m_pathRightFile = selectedFiles.at(1).url().path();
        m_pathCenterFile = m_config.readPathCenterFile();

        if (firstIsDir != isPathConsideredFolder(m_pathCenterFile))
        {
            m_pathCenterFile.clear();
        }
    }
    else
    {
        m_pathRightFile = selectedFiles.at(0).url().path();
        m_pathLeftFile = m_config.readPathLeftFile();
        m_pathCenterFile = m_config.readPathCenterFile();

        if (firstIsDir != isPathConsideredFolder(m_pathLeftFile))
        {
            m_pathLeftFile.clear();
        }

        if (firstIsDir != isPathConsideredFolder(m_pathCenterFile))
        {
            m_pathCenterFile.clear();
        }
    }

    CreateMenuCtx ctx;
    ctx.isDir = firstIsDir;
    ctx.menuType = BCompareConfig::MENU_SUBMENU;
    ctx.nbSelected = nbSelected;
    createMenus(listActions, ctx);

    if (listActions.size() > 0)
    {
        QMenu *subMenu = new QMenu();
        QAction *subMenuAction = new QAction(this);

        subMenuAction->setMenu(subMenu);
        subMenu->setTitle(i18n("Beyond Compare"));
        subMenu->setIcon(m_config.menuIconFull());
        subMenu->addActions(listActions);

        listActions.clear();
        listActions.append(subMenuAction);
    }

    ctx.menuType = BCompareConfig::MENU_MAIN;
    createMenus(listActions, ctx);

    return listActions;
}

K_PLUGIN_FACTORY_WITH_JSON(BCompareKde5Factory,
                           "bcompare_ext_kde5.json",
                           registerPlugin<BCompareKde5>();
                          )

#include "bcompare_ext_kde5.moc"