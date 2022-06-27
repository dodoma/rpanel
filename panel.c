#include "reef.h"

#include <event2/event.h>
#include <event2/event_struct.h>

#include "hiredis.h"
#include "hiredis/adapters/libevent.h"

#include "db.h"
#include "eflat.h"

#include<opencv2/opencv.hpp>

using namespace cv;
using namespace std;

typedef enum {
    PANEL_POINT = 0,
    PANEL_LINE,
    PANEL_POLYGON,
} ItemType;

typedef struct {
    ItemType type;
    time_t at;
    int duration;

    union {
        EPOINT point;
    } data;
} PanelItem;

MLIST *sntd;                    /* stuff need to be draw */
Mat image;

static pthread_t draw_thread;

static void* _drawImage(void *arg)
{
    mtc_mm_init("-", "draw", MTC_DEBUG);

    while (1) {
        int N = mlist_length(sntd);
        time_t now = time(NULL);
        vector<Point> points;

        mtc_mm_dbg("draw", "draw image %d", N);

        /* 1. 填充 */
        for (int i = 0; i < N; i++) {
            PanelItem *item = (PanelItem*)mlist_getx(sntd, i);
            switch (item->type) {
            case PANEL_POINT:
                if (item->duration == 0 || (now - item->at) < item->duration)
                    points.push_back(Point(item->data.point.x, item->data.point.y));
                break;
            default:
                break;
            }
        }

        /* 2. 重置 */
        image.setTo(cv::Scalar(0, 0, 0));

        /* 3. 填充 */
        for (int i = 0; i < points.size(); i++) {
            circle(image, points[i], 2, Scalar(255, 0, 255), 2, 8, 0);
        }
        polylines(image, points, false, Scalar(0, 255, 0), 1, 8, 0);
        points.clear();

        /* 4. 画图 */
        imshow("image", image);

        int r = waitKey(500);
        //printf("xxx %d\n", r);
        if (r == 82) {
            /* 'R' reset canvas, and clear ALL sntd */
            image.setTo(cv::Scalar(0, 0, 0));
            mlist_clear(sntd);
        }
    }

    return NULL;
}

static void _onSubscribe(redisAsyncContext *c, void *reply, void *privdata)
{
    MERR *err;
    redisReply *r = (redisReply*)reply;
    if (!r) return;

    struct event_base *ebase = (struct event_base*)privdata;

    if (r->type == REDIS_REPLY_ARRAY && r->elements == 3 && r->element[2]->str != NULL) {
        MLIST *alist;

        mtc_mm_dbg("main", "got message: %s", r->element[2]->str);

        err = mstr_array_split(&alist, r->element[2]->str, " ", 10);
        RETURN_NOK(err);

        if (mlist_length(alist) >= 4) {
            char *command = (char*)mlist_getx(alist, 0);
            int persist = atoi((char*)mlist_getx(alist, 1));
            float x = strtof((char*)mlist_getx(alist, 2), NULL);
            float y = strtof((char*)mlist_getx(alist, 3), NULL);

            if (!strcmp(command, "ADD")) {
                PanelItem *item = (PanelItem*)mos_calloc(1, sizeof(PanelItem));
                item->type = PANEL_POINT;
                item->at = time(NULL);
                item->duration = persist;
                item->data.point = {x, y};

                mlist_append(sntd, item);
            }
        } else mtc_mm_warn("main", "unknown message %s", r->element[2]->str);

        mlist_destroy(&alist);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 5) {
        printf("useage %s width x height redisHost redisPort\n", argv[0]);
        return 1;
    }

    int width   = atoi(argv[1]);
    int height  = atoi(argv[2]);
    char *host  = argv[3];
    int port    = atoi(argv[4]);
    MERR *err;

    mtc_mm_init("-", "main", MTC_DEBUG);

    mlist_init(&sntd, free);

    image = Mat::zeros(width, height, CV_8UC3);

    struct event_base *ebase = event_base_new();
    redisAsyncContext *chan = redisAsyncConnect(host, port);
    if (!chan || chan->err) {
        mtc_mm_err("main", "connect %s:%d async failure %s", host, port, chan->errstr);
        return 1;
    }
    redisLibeventAttach(chan, ebase);
    redisAsyncCommand(chan, _onSubscribe, (void*)ebase, "SUBSCRIBE RPANEL");

    mtc_mm_dbg("main", "main thread running");

    pthread_create(&draw_thread, NULL, _drawImage, NULL);

    event_base_dispatch(ebase);

    mtc_mm_dbg("main", "main thread over!");
    mlist_destroy(&sntd);

    return 0;
}
