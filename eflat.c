#include "reef.h"

#include "eflat.h"

double eflat_distance(EPOINT a, EPOINT b)
{
    double w = a.x - b.x;
    double h = a.y - b.y;

    return sqrt(w * w + h * h);
}

double eflat_angle(EPOINT a)
{
    double alpha = atan2(a.y, a.x);

    return RADIAN2DEGREE(alpha);
}

double eflat_angler(EPOINT a)
{
    return atan2(a.y, a.x);
}

double eflat_dot(EPOINT a, EPOINT b, EPOINT c)
{
    double ab_x = b.x - a.x;
    double ab_y = b.y - a.y;
    double ac_x = c.x - a.x;
    double ac_y = c.y - a.y;

    return ab_x * ac_x + ab_y * ac_y;
}

EPOINT eflat_position(double length, double angle)
{
    double x = length * cos(DEGREE2RADIAN(angle));
    double y = length * sin(DEGREE2RADIAN(angle));

    return {x, y};
}

/*
 * a = (n*C - B*D) / (n*A - B*B)
 * b = (A*D - B*C) / (n*A - B*B)
 * r = E / F
 * 其中：
 * A = sum(Xi * Xi)
 * B = sum(Xi)
 * C = sum(Xi * Yi)
 * D = sum(Yi)
 * E = sum((Xi - Xmean)*(Yi - Ymean))
 * F = sqrt(sum((Xi - Xmean)*(Xi - Xmean))) * sqrt(sum((Yi - Ymean)*(Yi - Ymean)))
 */
double eflat_line_fit(EPOLYGON p, double *a, double *b)
{
    float A, B, C, D, E, F;
    A = B = C = D = E = F = 0.0;

    for (int i = 0; i < p.numpoint; i++) {
        A += p.points[i].x * p.points[i].x;
        B += p.points[i].x;
        C += p.points[i].x * p.points[i].y;
        D += p.points[i].y;
    }

    // 计算斜率和截距
    float x = (p.numpoint * A) - (B * B);
    if (x == 0) {
        if (a) *a = 1;
        if (b) *b = 0;
    } else {
        if (a) *a = (p.numpoint * C - B * D) / x;
        if (b) *b = (A * D - B * C) / x;
    }

    // 计算相关系数
    float xmean = B / p.numpoint;
    float ymean = D / p.numpoint;
    float xsum = 0.0, ysum = 0.0;
    for (int i = 0; i < p.numpoint; i++) {
        xsum += (p.points[i].x - xmean) * (p.points[i].x - xmean);
        ysum += (p.points[i].y - ymean) * (p.points[i].y - ymean);
        E += (p.points[i].x - xmean) * (p.points[i].y - ymean);
    }
    F = sqrt(xsum) * sqrt(ysum);

    if (F > 0) return E / F;
    else return 0;
}

double etriplet_angle(EPOINT a, EPOINT b, EPOINT c)
{
    double alpha = atan2(c.y - b.y, c.x - b.x);
    double ms = sin(alpha), mc = cos(alpha);

    EPOINT p = {mc * (a.x - b.x) + ms * (a.y - b.y), ms * (a.x - b.x) * -1 + mc * (a.y - b.y)};

    return eflat_angle(p);
}

double etriplet_angles(EPOINT a, EPOINT b, EPOINT c)
{
    double alpha = acos(eflat_dot(b, a, c) / (eflat_distance(a, b) * eflat_distance(b, c)));

    return RADIAN2DEGREE(alpha);
}

bool etriplet_onsegment(EPOINT a, EPOINT c, EPOINT b)
{
    if (c.x <= EMAX(a.x, b.x) && c.x >= EMIN(a.x, b.x) &&
        c.y <= EMAX(a.y, b.y) && c.y >= EMIN(a.y, b.y))
        return true;
    else return false;
}

EORIENT etriplet_orientation(EPOINT a, EPOINT b, EPOINT c)
{
    double val = (b.y - a.y) * (c.x - b.x) - (b.x - a.x) * (c.y - b.y);

    if (DOUBLE_EQUAL(val, 0)) return EORIENT_COLLINEAR;
    else if (val > 0) return EORIENT_CLOCKWISE;
    else return EORIENT_COUNTERCLOCK;
}

bool eline_intresect(ELINE m, ELINE n)
{
    EORIENT o1 = etriplet_orientation(m.a, m.b, n.a);
    EORIENT o2 = etriplet_orientation(m.a, m.b, n.b);
    EORIENT o3 = etriplet_orientation(n.a, n.b, m.a);
    EORIENT o4 = etriplet_orientation(n.a, n.b, m.b);

    /* 正常交叉 */
    if (o1 != o2 && o3 != o4) return true;

    /* n.a 在线段 m 上 */
    if (o1 == EORIENT_COLLINEAR && etriplet_onsegment(m.a, n.a, m.b)) return true;

    /* n.b 在线段 m 上 */
    if (o2 == EORIENT_COLLINEAR && etriplet_onsegment(m.a, n.b, m.b)) return true;

    /* m.a 在线段 n 上 */
    if (o3 == EORIENT_COLLINEAR && etriplet_onsegment(n.a, m.a, n.b)) return true;

    /* m.b 在线段 n 上 */
    if (o4 == EORIENT_COLLINEAR && etriplet_onsegment(n.a, m.b, n.b)) return true;

    return false;
}

EPOINT epolygon_vertex(EPOLYGON p, EDIRECT dir)
{
    EPOINT r = {0, 0};

    switch (dir) {
    case EDIRECT_X_UP:
        for (int i = 0; i < p.numpoint; i++) {
            if (p.points[i].x > r.x) r = p.points[i];
        }
        break;
    case EDIRECT_X_DOWN:
        for (int i = 0; i < p.numpoint; i++) {
            if (p.points[i].x < r.x) r = p.points[i];
        }
        break;
    case EDIRECT_Y_UP:
        for (int i = 0; i < p.numpoint; i++) {
            if (p.points[i].y > r.y) r = p.points[i];
        }
        break;
    case EDIRECT_Y_DOWN:
        for (int i = 0; i < p.numpoint; i++) {
            if (p.points[i].y < r.y) r = p.points[i];
        }
        break;
    default:
        break;
    }

    return r;
}

bool epolygon_inside(EPOINT a, EPOLYGON p)
{
    if (p.numpoint < 3) return false;

    /* 平行于X轴，当前点正向延伸到无限远 */
    EPOINT extreme = {INFINITE_X, a.y};

    int count = 0, i = 0, next = 0;

    while (i < p.numpoint) {
        /* 以三点为例，需要处理: 0, 1  1, 2  2, 0 */
        next = (i + 1) % p.numpoint;

        /* 两线段是否相交 */
        if (eline_intresect({p.points[i], p.points[next]}, {a, extreme})) {
            count++;

            /* 点 a 与线段在同一条直线上，线段内为界内，线段外为界外 */
            if (etriplet_orientation(p.points[i], a, p.points[next]) == 0)
                return etriplet_onsegment(p.points[i], a,  p.points[next]);

            /* 顶点与延长线共线，只计入一次计数 */
            if (etriplet_onsegment(a, p.points[i], extreme)) {
                count--;

                /* 最高、最低顶点与延长线相交，为界外 */
                if (DOUBLE_EQUAL(epolygon_vertex(p, EDIRECT_Y_UP).y, p.points[i].y) ||
                    DOUBLE_EQUAL(epolygon_vertex(p, EDIRECT_Y_DOWN).y, p.points[i].y))
                    return false;
            }
        }

        i++;
    }

    /* 交点个数为奇数返回 true */
    return count & 1;
}

double egeo_distance(GPOINT a, GPOINT b)
{
    double m = DEGREE2RADIAN(a.lat);
    double n = DEGREE2RADIAN(b.lat);

    double o = DEGREE2RADIAN(b.lat - a.lat);
    double p = DEGREE2RADIAN(b.lon - a.lon);

    double x = sin(o/2) * sin(o/2) + cos(m) * cos(n) * sin(p/2) * sin(p/2);
    double c = 2 * atan2(sqrt(x), sqrt(1-x));

    return EARTH_R * c;
}

GPOINT egeo_move(GPOINT a, double distance, double angle)
{
    double dx = distance * cos(DEGREE2RADIAN(angle));
    // One degree of longitude equals 111321 meters (at the equator)
    double dlon = dx / (111321 * cos(DEGREE2RADIAN(a.lat)));

    double dy = distance * sin(DEGREE2RADIAN(angle));
    // One degree of latitude on the Earth's surface equals (110574 meters)
    double dlat = dy / 110574;

    return {a.lon + dlon, a.lat + dlat};
}
