#include "../plugins/qml/QtPresentationTypes.hpp"

#include <array>
#include <cstdio>

int main() {
    const QSizeF                 bounds(400, 200);
    const std::array<QPointF, 8> expected {
        QPointF(0.2, 0.7), QPointF(0.3, 0.2), QPointF(0.8, 0.3), QPointF(0.7, 0.8),
        QPointF(0.8, 0.7), QPointF(0.7, 0.2), QPointF(0.2, 0.3), QPointF(0.3, 0.8),
    };
    for (quint32 transform = 0; transform < expected.size(); ++transform) {
        const QSizeF pre    = (transform & 1u) ? QSizeF(200, 400) : bounds;
        const auto   matrix = qtCompositionTransform(transform, bounds);
        const auto   point  = matrix.map(QPointF(pre.width() * 0.2, pre.height() * 0.7));
        if (qAbs(point.x() / bounds.width() - expected[transform].x()) > 0.00001 ||
            qAbs(point.y() / bounds.height() - expected[transform].y()) > 0.00001) {
            std::fprintf(stderr, "incorrect composition transform %u\n", transform);
            return 1;
        }
        const auto rect = matrix.mapRect(QRectF(QPointF(), pre));
        if (qAbs(rect.x()) > 0.0001 || qAbs(rect.y()) > 0.0001 ||
            qAbs(rect.width() - bounds.width()) > 0.0001 ||
            qAbs(rect.height() - bounds.height()) > 0.0001)
            return 1;
    }
    return 0;
}
