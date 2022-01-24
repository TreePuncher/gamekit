#include "SceneOutliner.h"
#include <qtimer.h>
#include <chrono>
#include "EditorViewport.h"

using namespace std::chrono_literals;


/************************************************************************************************/


SceneOutliner::SceneOutliner(EditorViewport& IN_viewport, QWidget *parent) :
    QWidget     { parent },
    dataModel   { IN_viewport.GetScene() },
    viewport    { IN_viewport }
{
	ui.setupUi(this);

    timer = new QTimer{ this };
    connect(timer, &QTimer::timeout, this, &SceneOutliner::Update);
    timer->setTimerType(Qt::PreciseTimer);
    timer->start(500ms);

    ui.listView->setModel(&dataModel);
    ui.listView->setResizeMode(QListView::ResizeMode::Adjust);
    ui.listView->setGridSize({ 50, 25 });

    ui.listView->setEditTriggers(QAbstractItemView::AnyKeyPressed | QAbstractItemView::DoubleClicked);

    connect(ui.listView, &QListView::clicked, this, &SceneOutliner::on_clicked);
}


/************************************************************************************************/


SceneOutliner::~SceneOutliner()
{
}


/************************************************************************************************/


void SceneOutliner::Update()
{
    dataModel.scene = viewport.GetScene();

    ui.listView->update();

    timer->start(500ms);
}


/************************************************************************************************/


void SceneOutliner::on_clicked()
{
    auto idx = ui.listView->currentIndex();
    auto& selectionCtx = viewport.GetSelectionContext();
    auto& object = dataModel.scene->sceneObjects[idx.row()];

    ViewportObjectList selection;
    selection.push_back(object);

    selectionCtx.selection  = std::move(selection);
    selectionCtx.type       = ViewportObjectList_ID;
}


/************************************************************************************************/
