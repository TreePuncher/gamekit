#pragma once

#include <QWidget>
#include "ui_EditorAnimationEditor.h"


/************************************************************************************************/


class AnimatorSelectionContext;
class EditorPrefabPreview;
class EditorRenderer;
class EditorProject;
class EditorCodeEditor;
class QMenuBar;
class EditorScriptEngine;
class SelectionContext;
class EditorPrefabInputTab;
class QTimer;


/************************************************************************************************/


namespace FlexKit
{
    class Animation;
}

class EditorPrefabEditor : public QWidget
{
	Q_OBJECT

public:
    EditorPrefabEditor(SelectionContext& IN_selection, EditorScriptEngine& engine, EditorRenderer& IN_renderer, EditorProject& IN_project, QWidget *parent = Q_NULLPTR);
	~EditorPrefabEditor();

private:

    FlexKit::Animation* LoadAnimation       (std::string&, bool);
    void                ReleaseAnimation    (FlexKit::Animation*);

	Ui::EditorAnimationEditor               ui;
    SelectionContext&                       globalSelection;
    QMenuBar*                               menubar;
    AnimatorSelectionContext*               localSelection;
    EditorPrefabPreview*                    previewWindow;
    EditorPrefabInputTab*                   inputVariables;
    EditorProject&                          project;
    EditorCodeEditor*                       codeEditor;
    EditorRenderer&                         renderer;
    EditorScriptEngine&                     scriptEngine;
    QTimer*                                 timer;
};

/**********************************************************************

Copyright (c) 2021 - 2022 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/
