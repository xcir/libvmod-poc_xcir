static const char VMOD_PARAM_HEADER[] = "\030x-vmod-smalllight-param:";



enum vmod_http_small_light_output_format{
	VMOD_HTTP_SMALL_LIGHT_OF_JPEG,
	VMOD_HTTP_SMALL_LIGHT_OF_PNG,
	VMOD_HTTP_SMALL_LIGHT_OF_TIFF,
	VMOD_HTTP_SMALL_LIGHT_OF_GIF,
	VMOD_HTTP_SMALL_LIGHT_OF_AUTO
};

enum vmod_http_small_light_pass_through{
	VMOD_HTTP_SMALL_LIGHT_PT_PTSS,
	VMOD_HTTP_SMALL_LIGHT_PT_PTLS,
	VMOD_HTTP_SMALL_LIGHT_PT_NOPE
};

enum vmod_http_small_light_aspect_ratio{
	VMOD_HTTP_SMALL_LIGHT_DA_SHORT_EDGE,
	VMOD_HTTP_SMALL_LIGHT_DA_LONG_EDGE,
	VMOD_HTTP_SMALL_LIGHT_DA_NOPE
};
enum vmod_http_small_light_scale{
	VMOD_HTTP_SMALL_LIGHT_DS_FORCE_SCALE,
	VMOD_HTTP_SMALL_LIGHT_DS_NO_SCALE_SMALL_IMAGE,
};
	
enum vmod_http_small_light_engine{
	//VMOD_HTTP_SMALL_LIGHT_E_IMLIB2,
	VMOD_HTTP_SMALL_LIGHT_E_IMAGEMAGICK,
	VMOD_HTTP_SMALL_LIGHT_E_DUMMY
};

enum vmod_http_small_light_coord_unit_t{
	VMOD_HTTP_SMALL_LIGHT_COORD_UNIT_NONE,
	VMOD_HTTP_SMALL_LIGHT_COORD_UNIT_PIXEL,
	VMOD_HTTP_SMALL_LIGHT_COORD_UNIT_PERCENT
};

struct vmod_http_small_light_coord_t{
	unsigned		magic;
	#define VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC	0x2aadace3
    double v;
    enum vmod_http_small_light_coord_unit_t u;
};

struct vmod_http_small_light_color_t{
	unsigned		magic;
	#define VMOD_HTTP_SMALL_LIGHT_COLOR_T_MAGIC	0xfaadace3
	short r;
	short g;
	short b;
	short a;
};

struct vmod_smalllight_param {
	unsigned		magic;
#define VMOD_SMALLLIGHT_PARAM_MAGIC	0x2a9daed2
	double    iw;
	double    ih;

	struct vmod_http_small_light_coord_t    *sx;
	struct vmod_http_small_light_coord_t    *sy;
	struct vmod_http_small_light_coord_t    *sw;
	struct vmod_http_small_light_coord_t    *sh;
	
	struct vmod_http_small_light_coord_t    *dx;
	struct vmod_http_small_light_coord_t    *dy;
	struct vmod_http_small_light_coord_t    *dw;
	struct vmod_http_small_light_coord_t    *dh;
	
	enum vmod_http_small_light_aspect_ratio  da;
	enum vmod_http_small_light_scale         ds;
	double                                   cw;
	double                                   ch;
	struct vmod_http_small_light_color_t    *cc;
	
	double                                   bw;
	double                                   bh;
	struct vmod_http_small_light_color_t    *bc;

	enum vmod_http_small_light_pass_through  pt;
	double                                   q;
	enum vmod_http_small_light_output_format of;
	
	unsigned                                 inhexif;
	unsigned                                 jpeghint;
	unsigned                                 info;
	enum vmod_http_small_light_engine        e;
	//sharpen / unsharp / blurはそれぞれのエンジン内でパースする
	
	double         aspect;
	unsigned       f_pt;
	unsigned       f_crop;
	unsigned       f_scale;
	
};

#define VMOD_SMALLLIGHT_PARAM_IGNORE_Q  -1

struct vmod_smalllight_param * vmod_smalllight_param_alloc();
void vmod_smalllight_param_free(struct vmod_smalllight_param *sml);
void vmod_smalllight_param_read(struct busyobj *bo, struct vmod_smalllight_param* pr);
void vmod_smalllight_param_calc(struct busyobj *bo, struct vmod_smalllight_param* pr);
