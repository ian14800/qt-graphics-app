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

#include "QtGraphicsTester.h"

#include "Camera.h"
#include "AttributeListModel.h"
#include "Envelope.h"
#include "Graphic.h"
#include "GraphicListModel.h"
#include "GraphicsOverlay.h"
#include "GraphicsOverlayListModel.h"
#include "LocalSceneQuickView.h"
#include "MapTypes.h"
#include "ModelSymbolLayer.h"
#include "MultilayerPointSymbol.h"
#include "Point.h"
#include "PortalItem.h"
#include "Scene.h"
#include "Renderer.h"
#include "RendererSceneProperties.h"
#include "SimpleMarkerSymbol.h"
#include "SimpleRenderer.h"
#include "SpatialReference.h"
#include "Symbol.h"
#include "SymbolStyle.h"
#include "SymbolTypes.h"
#include "Viewpoint.h"
#include "Error.h"
#include "ErrorException.h"
#include "SymbolStyleSearchParameters.h"
#include "SymbolStyleSearchResult.h"
#include "SymbolStyleSearchResultListModel.h"

#include <cmath>
#include <QPointer>
#include <QDebug>
#include <QFuture>
#include <QRandomGenerator>
#include <QTimer>
#include <QStringList>
#include <qqml.h>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

using namespace Esri::ArcGISRuntime;

QtGraphicsTester::QtGraphicsTester(QObject* parent /* = nullptr */):
    QObject(parent)
{
}

void QtGraphicsTester::init() {
    qmlRegisterType<Esri::ArcGISRuntime::LocalSceneQuickView>("Esri.QtGraphicsTester", 1, 0, "LocalSceneView");
    qmlRegisterType<QtGraphicsTester>("Esri.QtGraphicsTester", 1, 0, "QtGraphicsTester");
}

QtGraphicsTester::~QtGraphicsTester() = default;

QString QtGraphicsTester::diagnosticsText() const
{
    return m_diagnosticsText;
}

LocalSceneQuickView* QtGraphicsTester::sceneView() const
{
    return m_sceneView;
}

void QtGraphicsTester::setSceneView(LocalSceneQuickView* sceneView)
{
    if (!sceneView || sceneView == m_sceneView)
    {
        return;
    }

    m_sceneView = sceneView;
    initializeSceneAndOverlay();
    updateDiagnostics();

    emit sceneViewChanged();
}

namespace
{
GraphicsOverlay* findMainOverlay(GraphicsOverlayListModel* overlays)
{
    if (!overlays)
    {
        return nullptr;
    }

    for (int i = 0; i < overlays->size(); ++i)
    {
        GraphicsOverlay* candidate = overlays->at(i);
        if (candidate && candidate->overlayId() == QStringLiteral("qt-graphics-tester-main-overlay"))
        {
            return candidate;
        }
    }

    return nullptr;
}

QString rendererTypeToString(Renderer* renderer)
{
    if (!renderer)
    {
        return QStringLiteral("none");
    }

    switch (renderer->rendererType())
    {
    case RendererType::SimpleRenderer:
        return QStringLiteral("SimpleRenderer");
    case RendererType::UniqueValueRenderer:
        return QStringLiteral("UniqueValueRenderer");
    case RendererType::ClassBreaksRenderer:
        return QStringLiteral("ClassBreaksRenderer");
    case RendererType::DictionaryRenderer:
        return QStringLiteral("DictionaryRenderer");
    case RendererType::HeatmapRenderer:
        return QStringLiteral("HeatmapRenderer");
    case RendererType::UnsupportedRenderer:
        return QStringLiteral("UnsupportedRenderer");
    default:
        return QStringLiteral("UnknownRenderer");
    }
}

QString runtimeCoreDiagnostics()
{
#if defined(Q_OS_WIN)
    HMODULE module = GetModuleHandleW(L"runtimecore.dll");
    if (!module)
    {
        return QStringLiteral("runtimecore.dll: not loaded");
    }

    wchar_t modulePath[32768]{};
    const DWORD pathLength = GetModuleFileNameW(module, modulePath, static_cast<DWORD>(sizeof(modulePath) / sizeof(modulePath[0])));
    if (pathLength == 0 || pathLength >= sizeof(modulePath) / sizeof(modulePath[0]))
    {
        return QStringLiteral("runtimecore.dll: loaded, path unavailable");
    }

    const QFileInfo fileInfo(QString::fromWCharArray(modulePath, static_cast<qsizetype>(pathLength)));
    return QStringLiteral("runtimecore.dll modified date: %1")
        .arg(
             fileInfo.lastModified().toString(Qt::ISODateWithMs)
            );
#else
    return {};
#endif
}
}

bool QtGraphicsTester::initializeSceneAndOverlay()
{
    if (!m_sceneView)
    {
        qWarning() << "QtGraphicsTester requires sceneView to be set before initialization.";
        return false;
    }

    if (!m_scene)
    {
        // Local
        PortalItem* item = new PortalItem(QStringLiteral("407edbd836ef43e6911f39366976bb7c"), this);
        // Global
        // PortalItem* item = new PortalItem(QStringLiteral("cee911a0adea4bb69a2df1d99ec4bd5f"), this);
        m_scene = new Scene(item, this);
    }

    if (m_sceneView->arcGISScene() != m_scene)
    {
        m_sceneView->setArcGISScene(m_scene);
    }

    GraphicsOverlayListModel* overlays = m_sceneView->graphicsOverlays();

    if (!overlays)
    {
        qWarning() << "Scene view does not have a graphics overlay list yet.";
        return false;
    }

    m_graphicsOverlay = findMainOverlay(overlays);

    if (!m_graphicsOverlay)
    {
        m_graphicsOverlay = new GraphicsOverlay(this);
        m_graphicsOverlay->setOverlayId(QStringLiteral("qt-graphics-tester-main-overlay"));
        m_graphicsOverlay->setRenderingMode(GraphicsRenderingMode::Dynamic);
        overlays->append(m_graphicsOverlay);

        qDebug() << "Graphics overlay rendering mode set to Dynamic for immediate update/removal redraw.";
    }

    applyCurrentRendererToOverlay();

    qDebug() << "Overlay state: ptr=" << static_cast<void*>(m_graphicsOverlay)
             << "id=" << m_graphicsOverlay->overlayId()
             << "totalOverlays=" << overlays->size();

    updateDiagnostics();

    return true;
}

bool QtGraphicsTester::ensureSceneAndOverlay()
{
    if (!m_sceneView)
    {
        qWarning() << "QtGraphicsTester requires sceneView to be set before use.";
        return false;
    }

    if (!m_scene || m_sceneView->arcGISScene() != m_scene)
    {
        return initializeSceneAndOverlay();
    }

    GraphicsOverlayListModel* overlays = m_sceneView->graphicsOverlays();
    if (!overlays)
    {
        qWarning() << "Scene view does not have a graphics overlay list yet.";
        return false;
    }

    if (!m_graphicsOverlay || !overlays->contains(m_graphicsOverlay))
    {
        return initializeSceneAndOverlay();
    }

    return true;
}

QString QtGraphicsTester::readRendererJson(QString file_path)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream in(&file);
    QString jsonString = in.readAll();

    file.close();

    return jsonString;
}

bool QtGraphicsTester::applyCurrentRendererToOverlay()
{
    if (!m_graphicsOverlay)
    {
        return false;
    }

    Renderer* renderer = nullptr;
    Symbol* symbol = nullptr;
    constexpr float markerSize = 50.0f;

    if (m_currentRendererId == QStringLiteral("sRenderer1"))
    {
        symbol = new SimpleMarkerSymbol(SimpleMarkerSymbolStyle::Circle, QColor("red"), markerSize, this);
    }
    else if (m_currentRendererId == QStringLiteral("sRenderer2"))
    {
        ModelSymbolLayer* coneLayer = new ModelSymbolLayer(ModelShape::Cone, this);
        coneLayer->setColor(QColor("yellow"));
        coneLayer->setWidth(markerSize);
        coneLayer->setDepth(markerSize);
        coneLayer->setHeight(markerSize * 2.0);
        symbol = new MultilayerPointSymbol({coneLayer}, this);
    }
    else if (m_currentRendererId == QStringLiteral("webStyleRenderer"))
    {
        // auto* symbolStyle = new SymbolStyle(QStringLiteral("EsriThematicShapesStyle"), nullptr, this);
        // symbolStyle->fetchSymbolAsync({QStringLiteral("Standing Tetrahedron")}, symbolStyle)
        //     .then(this, [this](Symbol* webStyleSymbol)
        auto* symbolStyle = SymbolStyle::createSymbolStyleFromUrl(
            QUrl(QStringLiteral(
                // Pushpins
                // "https://www.arcgis.com/home/item.html?id=37da62fcdb854f8e8305c79e8b5023dc")),
                // Government
                "https://www.arcgis.com/home/item.html?id=6eeef46c653b40c9bda04f9bed913b70")),
                // Animals
                // "https://www.arcgis.com/home/item.html?id=1fbb242c54e4415d9b8e8a343ca7a9d0")),
            this);

        // No filters: search the entire style.
        SymbolStyleSearchParameters parameters;

        symbolStyle->searchSymbolsAsync(parameters, symbolStyle)
            .then(this, [this, symbolStyle](SymbolStyleSearchResultListModel* results)
        {
            if (m_currentRendererId != QStringLiteral("webStyleRenderer") ||
                !m_graphicsOverlay)
            {
                return;
            }

            if (!results || results->isEmpty())
            {
                qWarning() << "The web style returned no symbols.";
                return;
            }

            const auto entries = results->searchResults();
            auto* graphics = m_graphicsOverlay->graphics();
            const int count = qMin(graphics->size(), results->size());

            for (int i = 0; i < count; ++i)
            {
                QPointer<Graphic> graphic(graphics->at(i));
                if (!graphic)
                    continue;

                const QString key = entries.at(i).key();

                symbolStyle->fetchSymbolAsync({key}, symbolStyle)
                    .then(this, [this, graphic, key](Symbol* symbol)
                {
                    if (!graphic ||
                        m_currentRendererId != QStringLiteral("webStyleRenderer") ||
                        !m_graphicsOverlay ||
                        !m_graphicsOverlay->graphics()->contains(graphic.data()))
                    {
                        return;
                    }

                    auto* pointSymbol = qobject_cast<MultilayerPointSymbol*>(symbol);
                    if (!pointSymbol)
                    {
                        qWarning() << "Not a multilayer point symbol:" << key;
                        return;
                    }

                    pointSymbol->setSize(150.0f);
                    graphic->setSymbol(pointSymbol);

                    requestOverlayRedraw();
                    updateDiagnostics();
                })
                .onFailed(this, [key](const ErrorException& exception)
                {
                    qWarning() << "Failed to fetch symbol:" << key
                            << exception.error().message();
                });
            }
        })
        .onFailed(this, [](const ErrorException& exception)
        {
            qWarning() << "Failed to search web style:"
                    << exception.error().message();
        });

        // qDebug() << "Fetching web style symbol: Standing Tetrahedron";
        return true;
    }
    else if (m_currentRendererId == QStringLiteral("vvRenderer"))
    {
        QString rendererJsonString = readRendererJson("C:\\code\\dotnet\\NETGraphicsTester\\NETGraphicsTester\\Resources\\Raw\\visual_variables_renderer.json");
        renderer = Renderer::fromJson(rendererJsonString, this);

    }
    else if (m_currentRendererId == QStringLiteral("uvRenderer") ||
             m_currentRendererId == QStringLiteral("cbRenderer") ||
             m_currentRendererId == QStringLiteral("cusRenderer"))
    {
        qWarning() << "Renderer not implemented in this pass:" << m_currentRendererId;
        return false;
    }
    else
    {
        qWarning() << "Unknown renderer id:" << m_currentRendererId;
        return false;
    }

    if (!renderer)
    {
        renderer = new SimpleRenderer(symbol, this);
    }

    m_graphicsOverlay->setRenderer(renderer);

    const int graphicCount = m_graphicsOverlay->graphics()->size();
    for (int i = 0; i < graphicCount; ++i)
    {
        Graphic* graphic = m_graphicsOverlay->graphics()->at(i);
        if (graphic)
        {
            graphic->setSymbol(nullptr);
        }
    }

    qDebug() << "Applied overlay renderer:" << m_currentRendererId
             << "and cleared symbol overrides for" << graphicCount << "graphics.";
    requestOverlayRedraw();
    updateDiagnostics();
    return true;
}

void QtGraphicsTester::requestOverlayRedraw()
{
    if (!m_graphicsOverlay)
    {
        return;
    }

    const bool wasVisible = m_graphicsOverlay->isVisible();
    m_graphicsOverlay->setVisible(!wasVisible);
    m_graphicsOverlay->setVisible(wasVisible);
}

namespace
{
struct MoveVector
{
    double dx = 0.0;
    double dy = 0.0;
    double dz = 0.0;
    const char* description = "none";
};

double randomInRange(double minValue, double maxValue)
{
    return minValue + ((maxValue - minValue) * QRandomGenerator::global()->generateDouble());
}

MoveVector buildMoveVector(int moveDirection, double moveMagnitude)
{
    switch (moveDirection)
    {
    case 0:
        return {moveMagnitude, 0.0, 0.0, "positive x"};
    case 1:
        return {-moveMagnitude, 0.0, 0.0, "negative x"};
    case 2:
        return {0.0, moveMagnitude, 0.0, "positive y"};
    case 3:
        return {0.0, -moveMagnitude, 0.0, "negative y"};
    case 4:
        return {0.0, 0.0, moveMagnitude, "positive z"};
    case 5:
        return {0.0, 0.0, -moveMagnitude, "negative z"};
    default:
        return {0.0, 0.0, 0.0, "none"};
    }
}

bool applyMoveToGraphic(Graphic* graphic, const MoveVector& move)
{
    if (!graphic)
    {
        return false;
    }

    const Point currentPoint(graphic->geometry());
    if (!currentPoint.isValid())
    {
        return false;
    }

    const double currentZ = std::isfinite(currentPoint.z()) ? currentPoint.z() : 0.0;
    const Point movedPoint(
        currentPoint.x() + move.dx,
        currentPoint.y() + move.dy,
        currentZ + move.dz,
        currentPoint.spatialReference());

    graphic->setGeometry(movedPoint);
    return true;
}

SimpleMarkerSymbol* createTestSymbol(int styleIndex, QObject* parent)
{
    constexpr float markerSize = 11.0f;

    switch (styleIndex)
    {
    case 0:
        return new SimpleMarkerSymbol(SimpleMarkerSymbolStyle::Triangle, QColor("green"), markerSize, parent);
    case 1:
        return new SimpleMarkerSymbol(SimpleMarkerSymbolStyle::Circle, QColor("purple"), markerSize, parent);
    default:
        return new SimpleMarkerSymbol(SimpleMarkerSymbolStyle::Square, QColor("orange"), markerSize, parent);
    }
}
}

bool QtGraphicsTester::addGraphics(int count)
{
    if (count <= 0)
    {
        qWarning() << "addGraphics ignored because count must be positive:" << count;
        return false;
    }

    if (!ensureSceneAndOverlay())
    {
        return false;
    }

    const Camera currentCamera = m_sceneView->currentViewpointCamera();
    const Point currentLocation = currentCamera.location();
    const Viewpoint currentViewpoint = m_sceneView->currentViewpoint(ViewpointType::BoundingGeometry);
    const Envelope currentExtent = currentViewpoint.targetGeometry().extent();

    const bool hasValidExtent = currentExtent.isValid() && currentExtent.width() > 0.0 && currentExtent.height() > 0.0;
    SpatialReference pointSpatialReference = hasValidExtent
        ? currentExtent.spatialReference()
        : currentLocation.spatialReference();
    if (pointSpatialReference.isEmpty())
    {
        pointSpatialReference = SpatialReference::wgs84();
    }

    const double fallbackSpan = pointSpatialReference.isGeographic() ? 0.01 : 1000.0;
    const double xMin = hasValidExtent ? currentExtent.xMin() : currentLocation.x() - fallbackSpan;
    const double xMax = hasValidExtent ? currentExtent.xMax() : currentLocation.x() + fallbackSpan;
    const double yMin = hasValidExtent ? currentExtent.yMin() : currentLocation.y() - fallbackSpan;
    const double yMax = hasValidExtent ? currentExtent.yMax() : currentLocation.y() + fallbackSpan;

    const bool hasValidZRange = hasValidExtent && std::isfinite(currentExtent.zMin()) && std::isfinite(currentExtent.zMax()) && currentExtent.zMax() > currentExtent.zMin();
    const double zMin = hasValidZRange ? currentExtent.zMin() : 0.0;
    const double zMax = hasValidZRange ? currentExtent.zMax() : 100.0;

    // double xValue = xMin;
    // const double yValue = yMax - yMin;
    // const double zValue = zMax - zMin;
    // for (int i = 0; i < count; ++i)
    // {
    //     const Point point(
    //         xValue + (i * 10.0),
    //         yValue,
    //         zValue,
    //         pointSpatialReference);

    //     Graphic* graphic = new Graphic(point);
    //     graphic->attributes()->insertAttribute(QStringLiteral("render_class"), QRandomGenerator::global()->bounded(1, 11));
    //     graphic->attributes()->insertAttribute(QStringLiteral("rotation_value"), QRandomGenerator::global()->bounded(0, 360));
    //     m_graphicsOverlay->graphics()->append(graphic);
    // }
    
    for (int i = 0; i < count; ++i)
    {
        const Point point(
            randomInRange(xMin, xMax),
            randomInRange(yMin, yMax),
            randomInRange(zMin, zMax),
            pointSpatialReference);

        Graphic* graphic = new Graphic(point);
        graphic->attributes()->insertAttribute(QStringLiteral("render_class"), QRandomGenerator::global()->bounded(1, 11));
        graphic->attributes()->insertAttribute(QStringLiteral("rotation_value"), QRandomGenerator::global()->bounded(0, 360));
        m_graphicsOverlay->graphics()->append(graphic);
    }

    qDebug() << "Added graphics:" << count << "Total now:" << graphicsCount();
    updateDiagnostics();
    return true;
}

bool QtGraphicsTester::addGraphicsAndMove()
{

    if (!ensureSceneAndOverlay())
    {
        return false;
    }

    // Get viewpoint for moving later
    const Camera currentCamera = m_sceneView->currentViewpointCamera();
    const Point currentLocation = currentCamera.location();
    const Viewpoint currentViewpoint = m_sceneView->currentViewpoint(ViewpointType::BoundingGeometry);
    const Envelope currentExtent = currentViewpoint.targetGeometry().extent();
    
    const bool hasValidExtent = currentExtent.isValid() && currentExtent.width() > 0.0 && currentExtent.height() > 0.0;
    SpatialReference pointSpatialReference = hasValidExtent
    ? currentExtent.spatialReference()
    : currentLocation.spatialReference();
    if (pointSpatialReference.isEmpty())
    {
        pointSpatialReference = SpatialReference::wgs84();
    }
    
    // Add 10 graphics at 0, 0
    
    // Get range of current extent for moving graphics
    const double fallbackSpan = pointSpatialReference.isGeographic() ? 0.01 : 1000.0;
    const double xMin = hasValidExtent ? currentExtent.xMin() : currentLocation.x() - fallbackSpan;
    const double xMax = hasValidExtent ? currentExtent.xMax() : currentLocation.x() + fallbackSpan;
    const double yMin = hasValidExtent ? currentExtent.yMin() : currentLocation.y() - fallbackSpan;
    const double yMax = hasValidExtent ? currentExtent.yMax() : currentLocation.y() + fallbackSpan;
    
    const bool hasValidZRange = hasValidExtent && std::isfinite(currentExtent.zMin()) && std::isfinite(currentExtent.zMax()) && currentExtent.zMax() > currentExtent.zMin();
    const double zMin = hasValidZRange ? currentExtent.zMin() : 0.0;
    const double zMax = hasValidZRange ? currentExtent.zMax() : 100.0;
    
    for (int i = 0; i < 10; ++i)
    {
        const Point point(
            randomInRange(xMin, xMax),
            randomInRange(yMin, yMax),
            randomInRange(zMin, zMax),
            pointSpatialReference);

        Graphic* graphic = new Graphic(point);
        m_graphicsOverlay->graphics()->append(graphic);
    }
    // 1 second delay before moving graphics
    // QTimer::singleShot(1000, this, [this, xMin, xMax, yMin, yMax, zMin, zMax, pointSpatialReference]()
    // {
    //     if (!ensureSceneAndOverlay())
    //     {
    //         return;
    //     }

    //     const Point point(0, 0, 0, pointSpatialReference);
    //     // Random point in view

    //     Graphic* graphic = new Graphic(point);
    //     m_graphicsOverlay->graphics()->append(graphic);

    //     qDebug() << "Added graphics: 1 Total now:" << graphicsCount();
    //     updateDiagnostics();
    // });

    QTimer::singleShot(1000, this, [this, xMin, xMax, yMin, yMax, zMin, zMax, pointSpatialReference]()
    {
        if (!ensureSceneAndOverlay())
        {
            return;
        }

        GraphicListModel* graphics = m_graphicsOverlay->graphics();
        const int count = graphics->size();

        for (int i = 0; i < count; ++i)
        {
            // Random point in view
            // const Point point(
            //     randomInRange(xMin, xMax),
            //     randomInRange(yMin, yMax),
            //     randomInRange(zMin, zMax),
            //     pointSpatialReference);

            const Point point(10000000000, 0, 0, pointSpatialReference);

            // Move graphic to new location
            Graphic* graphic = graphics->at(i);
            if (graphic)
            {
                graphic->setGeometry(point);
            }
        }

        qDebug() << "Added graphics:" << count << "Total now:" << graphicsCount();
        updateDiagnostics();
    });

    return true;
}

bool QtGraphicsTester::removeGraphics(int count)
{
    if (count <= 0)
    {
        qWarning() << "removeGraphics ignored because count must be positive:" << count;
        return false;
    }

    if (!ensureSceneAndOverlay())
    {
        return false;
    }

    for (int i = 0; i < count; ++i)
    {
        const int lastIndex = m_graphicsOverlay->graphics()->size() - 1;
        if (lastIndex < 0)
        {
            break;
        }

        // Graphic* graphic = m_graphicsOverlay->graphics()->at(lastIndex);
        m_graphicsOverlay->graphics()->removeAt(lastIndex);
        // delete graphic;
    }

    qDebug() << "Removed graphics:" << count << "Total now:" << graphicsCount();
    requestOverlayRedraw();
    updateDiagnostics();
    return true;
}

bool QtGraphicsTester::setOverlayRenderer(const QString& rendererId)
{
    if (!ensureSceneAndOverlay())
    {
        return false;
    }

    m_currentRendererId = rendererId;
    return applyCurrentRendererToOverlay();
}

bool QtGraphicsTester::updateAllGraphics(const QString& updateId, int count)
{
    if (!ensureSceneAndOverlay())
    {
        return false;
    }

    if (count <= 0)
    {
        qWarning() << "updateAllGraphics ignored because count must be positive:" << count;
        return false;
    }

    GraphicListModel* graphics = m_graphicsOverlay->graphics();
    const int targetCount = std::min(count, graphics->size());

    if (updateId == QStringLiteral("updateRenderers"))
    {
        const QString nextRenderer = (m_currentRendererId == QStringLiteral("sRenderer1"))
            ? QStringLiteral("sRenderer2")
            : QStringLiteral("sRenderer1");

        return setOverlayRenderer(nextRenderer);
    }

    if (updateId == QStringLiteral("randomSymbols"))
    {
        for (int i = 0; i < targetCount; ++i)
        {
            Graphic* graphic = graphics->at(i);
            if (!graphic)
            {
                continue;
            }

            const int styleIndex = QRandomGenerator::global()->bounded(0, 3);
            graphic->setSymbol(createTestSymbol(styleIndex, this));
        }

        qDebug() << "Applied random per-graphic symbols to:" << targetCount << "graphics.";
        requestOverlayRedraw();
        updateDiagnostics();
        return true;
    }

    if (updateId == QStringLiteral("uniformSymbols"))
    {
        const int styleIndex = QRandomGenerator::global()->bounded(0, 3);
        SimpleMarkerSymbol* uniformSymbol = createTestSymbol(styleIndex, this);

        for (int i = 0; i < targetCount; ++i)
        {
            Graphic* graphic = graphics->at(i);
            if (!graphic)
            {
                continue;
            }

            graphic->setSymbol(uniformSymbol);
        }

        qDebug() << "Applied one shared symbol to:" << targetCount << "graphics.";
        requestOverlayRedraw();
        updateDiagnostics();
        return true;
    }

    if (updateId == QStringLiteral("uniformMove"))
    {
        const int moveDirection = QRandomGenerator::global()->bounded(0, 6);
        const double moveMagnitude = randomInRange(0.0, 100.0);
        const MoveVector move = buildMoveVector(moveDirection, moveMagnitude);
        int movedCount = 0;

        for (int i = 0; i < targetCount; ++i)
        {
            Graphic* targetGraphic = graphics->at(i);
            if (applyMoveToGraphic(targetGraphic, move))
            {
                ++movedCount;
            }
        }

        qDebug() << "Moved" << movedCount << "graphics in" << move.description << "direction.";
        requestOverlayRedraw();
        updateDiagnostics();
        return true;
    }

    if (updateId == QStringLiteral("randomMove"))
    {
        int movedCount = 0;

        for (int i = 0; i < targetCount; ++i)
        {
            const int moveDirection = QRandomGenerator::global()->bounded(0, 6);
            const double moveMagnitude = randomInRange(0.0, 100.0);
            const MoveVector move = buildMoveVector(moveDirection, moveMagnitude);

            Graphic* targetGraphic = graphics->at(i);
            if (applyMoveToGraphic(targetGraphic, move))
            {
                ++movedCount;
            }
        }

        qDebug() << "Moved" << movedCount << "graphics in random directions.";
        requestOverlayRedraw();
        updateDiagnostics();
        return true;
    }

    if (updateId == QStringLiteral("animatedMove"))
    {
        qWarning() << "animatedMove is not implemented yet.";
        return false;
    }

    qWarning() << "Update type not implemented yet:" << updateId;
    return false;
}

bool QtGraphicsTester::spinGraphics(const QString& spinType)
{
    if (!ensureSceneAndOverlay())
    {
        return false;
    }

    Renderer* renderer = m_graphicsOverlay->renderer();
    if (!renderer)
    {
        qWarning() << "Cannot spin graphics without an overlay renderer.";
        return false;
    }

    RendererSceneProperties sceneProperties = renderer->sceneProperties();
    QString currentExpression;

    if (spinType == QStringLiteral("roll"))
    {
        currentExpression = sceneProperties.rollExpression();
    }
    else if (spinType == QStringLiteral("pitch"))
    {
        currentExpression = sceneProperties.pitchExpression();
    }
    else
    {
        qWarning() << "Unknown spin type:" << spinType;
        return false;
    }

    bool isNumeric = false;
    const double currentAngle = currentExpression.toDouble(&isNumeric);
    const double nextAngle = std::fmod((isNumeric ? currentAngle : 0.0) + 45.0, 360.0);
    const QString nextExpression = QString::number(nextAngle);

    if (spinType == QStringLiteral("roll"))
    {
        sceneProperties.setRollExpression(nextExpression);
    }
    else
    {
        sceneProperties.setPitchExpression(nextExpression);
    }

    renderer->setSceneProperties(sceneProperties);
    m_graphicsOverlay->setRenderer(renderer);
    // requestOverlayRedraw();
    qDebug() << "Set renderer" << spinType << "expression to" << nextExpression;
    return true;
}

bool QtGraphicsTester::moveGraphic(double x, double y)
{
    if (!ensureSceneAndOverlay())
    {
        return false;
    }

    if (!m_graphicsOverlay || m_graphicsOverlay->graphics()->isEmpty())
    {
        qWarning() << "No graphics to move.";
        return false;
    }

    Graphic* graphic = m_graphicsOverlay->graphics()->at(0);
    if (!graphic)
    {
        qWarning() << "Graphic is null.";
        return false;
    }

    // Assuming the graphic has a point geometry
    Point newPoint = Point(x, y, SpatialReference::wgs84());
    graphic->setGeometry(newPoint);

    requestOverlayRedraw();
    updateDiagnostics();
    return true;
}

bool QtGraphicsTester::moveCamera(double x, double y)
{
    if (!m_sceneView)
    {
        qWarning() << "No scene view attached.";
        return false;
    }

    const Camera camera = m_sceneView->currentViewpointCamera();
    const Point location(x, y, camera.location().z(), SpatialReference::wgs84());
    m_sceneView->setViewpointCameraAndWait(camera.moveTo(location));
    updateDiagnostics();
    return true;
}

void QtGraphicsTester::updateDiagnostics()
{
    if (!m_sceneView)
    {
        const QString newText = QStringLiteral("No scene view attached.");
        if (newText != m_diagnosticsText)
        {
            m_diagnosticsText = newText;
            emit diagnosticsTextChanged();
        }
        return;
    }

    GraphicsOverlayListModel* overlays = m_sceneView->graphicsOverlays();
    QStringList lines;
    lines << runtimeCoreDiagnostics();
    lines << QStringLiteral("Overlays on scene: %1").arg(overlays ? overlays->size() : 0);

    if (overlays)
    {
        for (int i = 0; i < overlays->size(); ++i)
        {
            GraphicsOverlay* overlay = overlays->at(i);
            if (!overlay)
            {
                lines << QStringLiteral("[%1] <null overlay>").arg(i);
                continue;
            }

            const QString overlayId = overlay->overlayId().isEmpty()
                ? QStringLiteral("<no id>")
                : overlay->overlayId();
            const int graphicCount = overlay->graphics() ? overlay->graphics()->size() : 0;
            const QString rendererName = rendererTypeToString(overlay->renderer());

            lines << QStringLiteral("[%1] id=%2 graphics=%3 renderer=%4")
                         .arg(i)
                         .arg(overlayId)
                         .arg(graphicCount)
                         .arg(rendererName);
        }
    }

    const QString newText = lines.join('\n');
    if (newText != m_diagnosticsText)
    {
        m_diagnosticsText = newText;
        emit diagnosticsTextChanged();
    }
}

int QtGraphicsTester::graphicsCount() const
{
    if (!m_graphicsOverlay)
    {
        return 0;
    }

    return m_graphicsOverlay->graphics()->size();
}