#ifndef __EFLAT_H__
#define __EFLAT_H__

/*
 * eflat.h 平面几何运算函数
 */

__BEGIN_DECLS

#define POLYGON_MAX_POINTS 1024  /* 多边形最多包含顶点个数 */
#define INFINITE_X 1.08e8        /* 正向无限远X坐标 180 * 60 * 10000 */

#define PI (3.141592653589793)
#define EARTH_R (6371.004e3)

#define DEGREE2RADIAN(degree) ((degree) / 180.0 * PI)
#define RADIAN2DEGREE(radias) ((radias) / PI * 180.0)

#define EMAX(m, n) ((m) > (n) ? (m) : (n))
#define EMIN(m, n) ((m) < (n) ? (m) : (n))

#define DOUBLE_EQUAL(a, b) (fabs((a) - (b)) < 1e-6)

typedef enum {
    EORIENT_COLLINEAR = 0,       /* 共线 */
    EORIENT_CLOCKWISE,           /* 顺时针方向 */
    EORIENT_COUNTERCLOCK         /* 逆时针方向 */
} EORIENT;

typedef enum {
    EDIRECT_X_UP = 0,
    EDIRECT_X_DOWN = 1,
    EDIRECT_Y_UP = 1 << 2,
    EDIRECT_Y_DOWN = 1 << 3
} EDIRECT;

/* 平面上点 */
typedef struct {
    double x;
    double y;
} EPOINT;

/* 平面上线段 */
typedef struct {
    EPOINT a;
    EPOINT b;
} ELINE;

/* 平面上多边形（必须安顺序存放） */
typedef struct {
    int numpoint;
    EPOINT points[POLYGON_MAX_POINTS];
} EPOLYGON;

/* 地球上点 */
typedef struct {
    double lon;                 /* 经度 */
    double lat;                 /* 纬度 */
} GPOINT;

/* 平面上 点 a 到 点 b 的距离 */
double eflat_distance(EPOINT a, EPOINT b);

/*
 * 点 a 的角度（度数表示）
 * a -> b 向量的角度，即为点(b.x - a.x, b.y - a.y) 的角度
 */
double eflat_angle(EPOINT a);
/* 点 a 的角度（弧度表示） */
double eflat_angler(EPOINT a);

/*
 * 点乘
 */
double eflat_dot(EPOINT a, EPOINT b, EPOINT c);

/*
 * 距离原点给定长度和角度 的 点坐标
 */
EPOINT eflat_position(double length, double angle);

/*
 * 多点拟合出一条直线 y = ax + b
 * 返回值为拟合系数r, r 的取值在[-1,1],
 * fabs(r) ==> 1 说明点线性关系好
 * fabs(r) ==> 0 说明点线性关系差，拟合无意义
 */
double eflat_line_fit(EPOLYGON p, double *a, double *b);

/*
 * 计算三点组成的夹角，线段 ab 与线段 bc 组成的 以 b 为顶点的夹角
 * 坐标转换方式，有正负
 */
double etriplet_angle(EPOINT a, EPOINT b, EPOINT c);

/*
 * 计算三点组成的夹角，线段 ab 与线段 bc 组成的 以 b 为顶点的夹角
 * 点乘公式方式，返回标量
 */
double etriplet_angles(EPOINT a, EPOINT b, EPOINT c);

/*
 * 三共线点中，c 是否落在线段 a b 上
 */
bool etriplet_onsegment(EPOINT a, EPOINT c, EPOINT b);

/*
 * 有序三点 a, b, c 的分布方向
 */
EORIENT etriplet_orientation(EPOINT a, EPOINT b, EPOINT c);

/*
 * 两条线段是否相交
 */
bool eline_intresect(ELINE m, ELINE n);

/*
 * 多边形 p 各方向上的顶点
 */
EPOINT epolygon_vertex(EPOLYGON p, EDIRECT dir);

/*
 * 点 a 是否在多边形 p 内
 */
bool epolygon_inside(EPOINT a, EPOLYGON p);

/*
 * 经纬度坐标作平面计算时，为避免相减、相乘后小于1e-6，需要调整为分数的 10000 倍，对数值放大 (x 60 x 10000)
 */
inline EPOINT egeo_in_minute(GPOINT a)
{
    return {a.lon * 6e5, a.lat * 6e5};
}

/*
 * 地球上，点 A 到 点B 的距离（米为单位）
 */
double egeo_distance(GPOINT a, GPOINT b);

/*
 * 地球上，以 A 为起点，以与经线（中国是由西向东，北向为正）方向夹角 angle，移动距离 distance（米）后，终点的经纬度
 */
GPOINT egeo_move(GPOINT a, double distance, double angle);

__END_DECLS
#endif
