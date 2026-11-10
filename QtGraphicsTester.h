// Copyright 2026 ESRI
//
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
//
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
//
// See the Sample code usage restrictions document for further information.
//

#ifndef QTGRAPHICSTESTER_H
#define QTGRAPHICSTESTER_H

#include "GraphicsOverlay.h"

namespace Esri {
namespace ArcGISRuntime {
class Scene;
class LocalSceneQuickView;
} // namespace ArcGISRuntime
} // namespace Esri

#include <QObject>
#include <QString>

Q_MOC_INCLUDE("LocalSceneQuickView.h")

class QtGraphicsTester : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Esri::ArcGISRuntime::LocalSceneQuickView* sceneView READ sceneView WRITE setSceneView NOTIFY sceneViewChanged)
    Q_PROPERTY(QString diagnosticsText READ diagnosticsText NOTIFY diagnosticsTextChanged)

public:
    explicit QtGraphicsTester(QObject* parent = nullptr);
    ~QtGraphicsTester() override;

    Esri::ArcGISRuntime::LocalSceneQuickView* sceneView() const;
    void setSceneView(Esri::ArcGISRuntime::LocalSceneQuickView* sceneView);

    QString diagnosticsText() const;

    Q_INVOKABLE bool addGraphics(int count);
    Q_INVOKABLE bool addGraphicsAndMove();
    Q_INVOKABLE bool removeGraphics(int count);
    Q_INVOKABLE bool setOverlayRenderer(const QString& rendererId);
    Q_INVOKABLE bool updateAllGraphics(const QString& updateId, int count);
    Q_INVOKABLE bool moveGraphic(double x, double y);
    Q_INVOKABLE bool moveCamera(double x, double y);
    Q_INVOKABLE int graphicsCount() const;
    Q_INVOKABLE bool spinGraphics(const QString&);
    Q_INVOKABLE QString readRendererJson(QString file_path);

    static void init();

signals:
    void sceneViewChanged();
    void diagnosticsTextChanged();

private:
    bool initializeSceneAndOverlay();
    bool ensureSceneAndOverlay();
    void updateDiagnostics();
    bool applyCurrentRendererToOverlay();
    void requestOverlayRedraw();

    Esri::ArcGISRuntime::Scene* m_scene = nullptr;
    Esri::ArcGISRuntime::LocalSceneQuickView* m_sceneView = nullptr;
    Esri::ArcGISRuntime::GraphicsOverlay* m_graphicsOverlay = nullptr;
    QString m_currentRendererId = QStringLiteral("sRenderer1");
    QString m_diagnosticsText;
};

#endif // QTGRAPHICSTESTER_H
