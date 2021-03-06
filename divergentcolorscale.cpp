#include "divergentcolorscale.h"

#include <cmath>

DivergentColorScale::DivergentColorScale(const QColor &color1,
                                         const QColor &colorMiddle,
                                         const QColor &color2)
    : ColorScale{{color1, colorMiddle, color2}}
{
}

QColor DivergentColorScale::color(float t) const
{
    if (t < m_min || t > m_max) {
        return QColor();
    }

    // normalize t
    t /= std::max(fabs(m_max), fabs(m_min));

    if (t > 0.0f) {
        return ColorScale::lerp(m_colors[1], m_colors[2], t);
    }
    if (t < 0.0f) {
        return ColorScale::lerp(m_colors[1], m_colors[0], -t);
    }
    return m_colors[1];
}

DivergentColorScale DivergentColorScale::builtin(BuiltinDivergentColorScale scale)
{
    switch (scale) {
    case RedGrayBlue:
        return DivergentColorScale(QColor(68,  68,  221),
                                   QColor(189, 189, 189),
                                   QColor(221,  68,  68));
        /*return ContinuousColorScale{
            QColor(221,  68,  68),
            QColor(220,  69,  68),
            QColor(220,  70,  69),
            QColor(220,  71,  70),
            QColor(220,  73,  71),
            QColor(220,  74,  72),
            QColor(220,  75,  73),
            QColor(220,  77,  74),
            QColor(220,  78,  75),
            QColor(220,  79,  76),
            QColor(220,  80,  76),
            QColor(220,  81,  77),
            QColor(220,  83,  78),
            QColor(220,  84,  79),
            QColor(220,  85,  80),
            QColor(220,  86,  81),
            QColor(220,  87,  82),
            QColor(220,  88,  83),
            QColor(220,  89,  84),
            QColor(220,  91,  85),
            QColor(220,  92,  85),
            QColor(220,  93,  86),
            QColor(220,  94,  87),
            QColor(220,  95,  88),
            QColor(220,  96,  89),
            QColor(220,  97,  90),
            QColor(220,  98,  91),
            QColor(219,  99,  92),
            QColor(219, 100,  93),
            QColor(219, 101,  94),
            QColor(219, 102,  95),
            QColor(219, 103,  95),
            QColor(219, 104,  96),
            QColor(219, 105,  97),
            QColor(219, 106,  98),
            QColor(219, 107, 99),
            QColor(219, 108, 100),
            QColor(218, 109, 101),
            QColor(218, 110, 102),
            QColor(218, 111, 103),
            QColor(218, 112, 104),
            QColor(218, 113, 105),
            QColor(218, 114, 106),
            QColor(218, 115, 107),
            QColor(218, 116, 108),
            QColor(217, 117, 108),
            QColor(217, 118, 109),
            QColor(217, 119, 110),
            QColor(217, 120, 111),
            QColor(217, 121, 112),
            QColor(217, 122, 113),
            QColor(217, 123, 114),
            QColor(216, 124, 115),
            QColor(216, 125, 116),
            QColor(216, 126, 117),
            QColor(216, 127, 118),
            QColor(216, 127, 119),
            QColor(215, 128, 120),
            QColor(215, 129, 121),
            QColor(215, 130, 122),
            QColor(215, 131, 123),
            QColor(215, 132, 124),
            QColor(214, 133, 125),
            QColor(214, 134, 126),
            QColor(214, 135, 127),
            QColor(214, 136, 127),
            QColor(214, 137, 128),
            QColor(213, 138, 129),
            QColor(213, 138, 130),
            QColor(213, 139, 131),
            QColor(213, 140, 132),
            QColor(212, 141, 133),
            QColor(212, 142, 134),
            QColor(212, 143, 135),
            QColor(212, 144, 136),
            QColor(211, 145, 137),
            QColor(211, 146, 138),
            QColor(211, 146, 139),
            QColor(211, 147, 140),
            QColor(210, 148, 141),
            QColor(210, 149, 142),
            QColor(210, 150, 143),
            QColor(209, 151, 144),
            QColor(209, 152, 145),
            QColor(209, 153, 146),
            QColor(208, 153, 147),
            QColor(208, 154, 148),
            QColor(208, 155, 149),
            QColor(207, 156, 150),
            QColor(207, 157, 151),
            QColor(207, 158, 152),
            QColor(206, 159, 153),
            QColor(206, 160, 154),
            QColor(206, 160, 155),
            QColor(205, 161, 156),
            QColor(205, 162, 157),
            QColor(205, 163, 158),
            QColor(204, 164, 159),
            QColor(204, 165, 160),
            QColor(203, 166, 161),
            QColor(203, 166, 162),
            QColor(203, 167, 163),
            QColor(202, 168, 164),
            QColor(202, 169, 165),
            QColor(201, 170, 166),
            QColor(201, 171, 167),
            QColor(201, 171, 168),
            QColor(200, 172, 169),
            QColor(200, 173, 170),
            QColor(199, 174, 171),
            QColor(199, 175, 172),
            QColor(198, 176, 173),
            QColor(198, 177, 174),
            QColor(197, 177, 175),
            QColor(197, 178, 176),
            QColor(196, 179, 177),
            QColor(196, 180, 178),
            QColor(195, 181, 179),
            QColor(195, 182, 180),
            QColor(194, 182, 181),
            QColor(194, 183, 182),
            QColor(193, 184, 183),
            QColor(193, 185, 184),
            QColor(192, 186, 185),
            QColor(192, 187, 186),
            QColor(191, 187, 187),
            QColor(190, 188, 188),
            QColor(190, 189, 189),
            QColor(189, 189, 190),
            QColor(189, 188, 190),
            QColor(188, 187, 190),
            QColor(187, 186, 191),
            QColor(187, 185, 191),
            QColor(186, 184, 191),
            QColor(185, 183, 191),
            QColor(185, 182, 192),
            QColor(184, 181, 192),
            QColor(184, 180, 192),
            QColor(183, 179, 193),
            QColor(182, 178, 193),
            QColor(182, 177, 193),
            QColor(181, 176, 194),
            QColor(180, 175, 194),
            QColor(180, 174, 194),
            QColor(179, 173, 194),
            QColor(178, 172, 195),
            QColor(178, 171, 195),
            QColor(177, 170, 195),
            QColor(176, 169, 196),
            QColor(176, 168, 196),
            QColor(175, 167, 196),
            QColor(174, 166, 196),
            QColor(174, 165, 197),
            QColor(173, 164, 197),
            QColor(172, 163, 197),
            QColor(172, 162, 197),
            QColor(171, 161, 198),
            QColor(170, 160, 198),
            QColor(170, 159, 198),
            QColor(169, 158, 199),
            QColor(168, 157, 199),
            QColor(167, 156, 199),
            QColor(167, 155, 199),
            QColor(166, 155, 200),
            QColor(165, 154, 200),
            QColor(165, 153, 200),
            QColor(164, 152, 200),
            QColor(163, 151, 201),
            QColor(162, 150, 201),
            QColor(162, 149, 201),
            QColor(161, 148, 201),
            QColor(160, 147, 202),
            QColor(159, 146, 202),
            QColor(159, 145, 202),
            QColor(158, 144, 202),
            QColor(157, 143, 203),
            QColor(156, 142, 203),
            QColor(155, 141, 203),
            QColor(155, 140, 203),
            QColor(154, 139, 204),
            QColor(153, 138, 204),
            QColor(152, 137, 204),
            QColor(152, 136, 204),
            QColor(151, 135, 205),
            QColor(150, 134, 205),
            QColor(149, 133, 205),
            QColor(148, 132, 205),
            QColor(147, 131, 206),
            QColor(147, 130, 206),
            QColor(146, 129, 206),
            QColor(145, 128, 206),
            QColor(144, 128, 207),
            QColor(143, 127, 207),
            QColor(142, 126, 207),
            QColor(142, 125, 207),
            QColor(141, 124, 208),
            QColor(140, 123, 208),
            QColor(139, 122, 208),
            QColor(138, 121, 208),
            QColor(137, 120, 209),
            QColor(136, 119, 209),
            QColor(135, 118, 209),
            QColor(134, 117, 209),
            QColor(133, 116, 209),
            QColor(133, 115, 210),
            QColor(132, 114, 210),
            QColor(131, 113, 210),
            QColor(130, 112, 210),
            QColor(129, 111, 211),
            QColor(128, 110, 211),
            QColor(127, 109, 211),
            QColor(126, 109, 211),
            QColor(125, 108, 212),
            QColor(124, 107, 212),
            QColor(123, 106, 212),
            QColor(122, 105, 212),
            QColor(121, 104, 212),
            QColor(120, 103, 213),
            QColor(119, 102, 213),
            QColor(118, 101, 213),
            QColor(117, 100, 213),
            QColor(115,  99, 213),
            QColor(114,  98, 214),
            QColor(113,  97, 214),
            QColor(112,  96, 214),
            QColor(111,  95, 214),
            QColor(110,  94, 215),
            QColor(109,  94, 215),
            QColor(108,  93, 215),
            QColor(106,  92, 215),
            QColor(105,  91, 215),
            QColor(104,  90, 216),
            QColor(103,  89, 216),
            QColor(101,  88, 216),
            QColor(100,  87, 216),
            QColor( 99,  86, 216),
            QColor( 98,  85, 217),
            QColor( 96,  84, 217),
            QColor( 95,  83, 217),
            QColor( 94,  82, 217),
            QColor( 92,  81, 217),
            QColor( 91,  80, 218),
            QColor( 89,  80, 218),
            QColor( 88,  79, 218),
            QColor( 86,  78, 218),
            QColor( 85,  77, 219),
            QColor( 83,  76, 219),
            QColor( 82,  75, 219),
            QColor( 80,  74, 219),
            QColor( 78,  73, 219),
            QColor( 77,  72, 220),
            QColor( 75,  71, 220),
            QColor( 73,  70, 220),
            QColor( 71,  69, 220),
            QColor( 69,  68, 220),
            QColor( 68,  68, 221)
        };*/
    default:
        return DivergentColorScale::builtin(DivergentColorScale::RedGrayBlue);
    }
}


DivergentColorScale *DivergentColorScale::builtin(BuiltinDivergentColorScale scale, void *)
{
    switch (scale) {
    case RedGrayBlue:
        return new DivergentColorScale(QColor(68,  68,  221),
                                       QColor(189, 189, 189),
                                       QColor(221,  68,  68));
    default:
        return DivergentColorScale::builtin(DivergentColorScale::RedGrayBlue, nullptr);
    }
}
