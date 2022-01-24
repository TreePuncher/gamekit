#pragma once

#include <QWidget>
#include "ui_SceneOutliner.h"
#include "ViewportScene.h"
#include <QStringListModel>

/************************************************************************************************/


class SceneItemModel : public QAbstractItemModel
{
public:
    SceneItemModel(std::shared_ptr<ViewportScene>& IN_scene) : scene{ IN_scene } {}

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const final
    {
        return createIndex(row, column);
    }


    QModelIndex parent(const QModelIndex& child) const final
    {
        return {};
    }


    int rowCount(const QModelIndex& parent = QModelIndex()) const final
    {
        if (scene)
            return scene->sceneObjects.size();
        else
            return 0;
    }


    int columnCount(const QModelIndex& parent = QModelIndex()) const final
    {
        if (scene)
            return 1;
        else 
            return 0;
    }


    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const final
    {
        if (role != Qt::DisplayRole)
            return {};

        switch (index.column())
        {
        case 0:
        {
            auto& projectSceneObject = scene->sceneObjects[index.row()];
            auto& gameObject = projectSceneObject->gameObject;
            if (auto id = FlexKit::GetStringID(gameObject); id)
                return { id };
            else 
                return { projectSceneObject->objectID };
        }
        default:
            return {};
        }
    }

    std::shared_ptr<ViewportScene>  scene;
};


/************************************************************************************************/


class EditorViewport;
class QTimer;

class SceneOutliner : public QWidget
{
	Q_OBJECT

public:
	SceneOutliner(EditorViewport&, QWidget *parent = Q_NULLPTR);
	~SceneOutliner();

private:

    void on_clicked();

    void Update();

	Ui::SceneOutliner               ui;

    EditorViewport&                 viewport;
    SceneItemModel                  dataModel;
    QTimer*                         timer;
};


/************************************************************************************************/
