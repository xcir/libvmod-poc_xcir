#include <syslog.h>
#include "vmod_poc_vfp.h"
#include <wand/MagickWand.h>


static const char VMOD_PARAM_HEADER[] = "\030x-vmod-smalllight-param:";

/*
Known problem
	- Should unset beresp.http.content-length
	- Should set the thread_pool_stack=512k in runtime-param




*/
//enum vmod_sml_work{
//	VMOD_SML_CROP = 1,
//	VMOD_SML_CANV = 1,
//}
/*
struct vmod_smalllight_param {
	unsigned				magic;
#define VMOD_SMALLLIGHT_PARAM_MAGIC		0x8d4cef02
	double iw, ih;
	double sx, sy, sw, sh;
	int    sxp,syp,swp,shp;
	double dx, dy, dw, dh;
	int    dxp,dyp,dwp,dhp;
	int    da;
	int    ds;
	double cw, ch;
	int    jpeg_hint;
	int    q;
	double baseline;//基準辺長
	int agif;//アニメgif有効
	int gpal;//global-pal
	//元サイズでのパススルー(1MB以上の時はスルーするみたいな

};
*/
enum vmod_http_small_light_aspect_ratio{
	VMOD_HTTP_SMALL_LIGHT_SHORT_EDGE,
	VMOD_HTTP_SMALL_LIGHT_LONG_EDGE,
	VMOD_HTTP_SMALL_LIGHT_NOPE
};
enum vmod_http_small_light_scale{
	VMOD_HTTP_SMALL_LIGHT_FORCE_SCALE,
	VMOD_HTTP_SMALL_LIGHT_NO_SCALE_SMALL_IMAGE,
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

struct vmod_poc_xcir_poc_xcir {
	unsigned		magic;
#define VMOD_POC_XCIR_POC_XCIR_MAGIC	0x2a9daed2
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
};


/*
	
	[clthread]req.http.x-vmod-smalllight-param
	[bgthread][vcl_backend_fetch]bereq0.http.x-vmod-smalllight-param -(copy)->bereq.http.x-vmod-smalllight-param ->REMOVE
	[bgthread] set to vfp
	[VFP] read from bereq0.http.x-vmod-smalllight-param
	
	
*/

const char *readParamRaw(struct busyobj *bo,const char* key){
	const char *p;
	http_GetHdrField(bo->bereq0, VMOD_PARAM_HEADER, key, &p);
	return p;
}
double parse_double(struct busyobj *bo,const char* key){
	const char *p;
	p = readParamRaw(bo, key);
	if(p==NULL) return 0;
	return atof(p);
}
int parse_coord(struct busyobj *bo,const char* key, struct vmod_http_small_light_coord_t *d){
	char *er;
	const char *p;
	p = readParamRaw(bo, key);
	syslog(6,"parse:%s %s",key,p);
	if(p==NULL || p[0]=='\0'){
		d->v = 0;
		d->u = VMOD_HTTP_SMALL_LIGHT_COORD_UNIT_NONE;
		return 0;
	}else{
		d->v = strtod(p,&er);
	}
	if(er[0]=='p'){
		d->u = VMOD_HTTP_SMALL_LIGHT_COORD_UNIT_PERCENT;
		return 0;
	}else{
		d->u = VMOD_HTTP_SMALL_LIGHT_COORD_UNIT_PIXEL;
		return 1;
	}
	return 1;
}
struct vmod_poc_xcir_poc_xcir * alloc_vmod_poc_xcir_poc_xcir(){
	struct vmod_poc_xcir_poc_xcir *sml;
	ALLOC_OBJ(sml, VMOD_POC_XCIR_POC_XCIR_MAGIC);
	AN(sml);

	
	
	///////////////////
	//ALLOC_OBJ(sml->iw, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	//AN(sml->iw);
	//ALLOC_OBJ(sml->ih, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	//AN(sml->ih);
	///////////////////
	ALLOC_OBJ(sml->sx, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	AN(sml->sx);
	ALLOC_OBJ(sml->sy, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	AN(sml->sy);
	ALLOC_OBJ(sml->sw, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	AN(sml->sw);
	ALLOC_OBJ(sml->sh, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	AN(sml->sh);
	///////////////////
	ALLOC_OBJ(sml->dx, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	AN(sml->dx);
	ALLOC_OBJ(sml->dy, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	AN(sml->dy);
	ALLOC_OBJ(sml->dw, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	AN(sml->dw);
	ALLOC_OBJ(sml->dh, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	AN(sml->dh);
	return sml;
}
void free_vmod_poc_xcir_poc_xcir(struct vmod_poc_xcir_poc_xcir *sml){
	CHECK_OBJ_NOTNULL(sml->sx, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	FREE_OBJ(sml->sx);
	CHECK_OBJ_NOTNULL(sml->sy, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	FREE_OBJ(sml->sy);
	CHECK_OBJ_NOTNULL(sml->sw, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	FREE_OBJ(sml->sw);
	CHECK_OBJ_NOTNULL(sml->sh, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	FREE_OBJ(sml->sh);
	////////////////////////
	CHECK_OBJ_NOTNULL(sml->dx, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	FREE_OBJ(sml->dx);
	CHECK_OBJ_NOTNULL(sml->dy, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	FREE_OBJ(sml->dy);
	CHECK_OBJ_NOTNULL(sml->dw, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	FREE_OBJ(sml->dw);
	CHECK_OBJ_NOTNULL(sml->dh, VMOD_HTTP_SMALL_LIGHT_COORD_T_MAGIC);
	FREE_OBJ(sml->dh);
	////////////////////////

	CHECK_OBJ_NOTNULL(sml, VMOD_POC_XCIR_POC_XCIR_MAGIC);
	FREE_OBJ(sml);

}
VCL_VOID vmod_poc_xcir__init(const struct vrt_ctx *ctx, struct vmod_poc_xcir_poc_xcir **smlp, const char *vcl_name
	){

}
VCL_VOID vmod_poc_xcir__fini(struct vmod_poc_xcir_poc_xcir **smlp)
{

}

void readParam(struct busyobj *bo, struct vmod_poc_xcir_poc_xcir* pr){
	const char *p;
	parse_coord(bo,"sx",pr->sx);
	parse_coord(bo,"sy",pr->sy);
	parse_coord(bo,"sw",pr->sw);
	parse_coord(bo,"sh",pr->sh);

	parse_coord(bo,"dx",pr->dx);
	parse_coord(bo,"dy",pr->dy);
	parse_coord(bo,"dw",pr->dw);
	parse_coord(bo,"dh",pr->dh);

	p = readParamRaw(bo, "da");
	if(p==NULL || p[0]=='l'){
		pr->da = VMOD_HTTP_SMALL_LIGHT_LONG_EDGE;
	}else if(p[0]=='s'){
		pr->da = VMOD_HTTP_SMALL_LIGHT_SHORT_EDGE;
	}else if(p[0]=='n'){
		pr->da = VMOD_HTTP_SMALL_LIGHT_NOPE;
	}else{
		pr->da = VMOD_HTTP_SMALL_LIGHT_LONG_EDGE;
	}

	p = readParamRaw(bo, "ds");
	if(p==NULL || p[0]=='n'){
		pr->ds = VMOD_HTTP_SMALL_LIGHT_NO_SCALE_SMALL_IMAGE;
	}else if(p[0]=='f'){
		pr->ds = VMOD_HTTP_SMALL_LIGHT_FORCE_SCALE;
	}else{
		pr->ds = VMOD_HTTP_SMALL_LIGHT_NO_SCALE_SMALL_IMAGE;
	}
	
	pr->cw = parse_double(bo,"cw");
	pr->ch = parse_double(bo,"ch");

}

void test(void** body,ssize_t *sz, struct busyobj *bo, struct vmod_poc_xcir_poc_xcir* pr){
	
	//これ増やさないと落ちる・・・
	// thread_pool_stack=512k

	
	
	MagickWand	*wand;
	//MagickWandGenesis();
	
	wand = NewMagickWand();
	
	//dw,dhを指定
	
	MagickReadImageBlob(wand, *body, *sz);
	MagickResetIterator(wand);
	
	//syslog(6,"%f",smlp->sx->v); 
	//smlp->iw = (double)MagickGetImageWidth(wand);
	//smlp->ih = (double)MagickGetImageWidth(wand);
	//prcalc(pr);
	
	//if(pr->sx < iw)
	readParam(bo,pr);
//	parse_coord(bo,"dw",pr->dw);
//	parse_coord(bo,"dh",pr->dh);

	syslog(6,"size:'%f' '%f'",pr->dw->v, pr->dh->v);
	while (MagickNextImage(wand) != MagickFalse){
		MagickResizeImage(wand, pr->dw->v, pr->dh->v ,LanczosFilter,1.0);
	}
	free(*body);
	
	//struct vmod_smalllight_buffer *buf;
	//buf = (struct vmod_smalllight_buffer *)vfe->priv1;
	size_t x;
		
	void *tmp = (void*)MagickGetImageBlob(wand, &x);
	*sz=x;
		syslog(6,"sz=%ld %ld",x,*sz);
	void *tmp2=calloc(*sz,1);
	memcpy(tmp2,tmp,*sz);
	AN(tmp2);
	*body = tmp2;
	DestroyMagickWand(wand);
	
	
}


int
init_function(struct vmod_priv *priv, const struct VCL_conf *conf)
{
	//for imagemagick
	putenv("MAGICK_THREAD_LIMIT=1");
	//テンポラリパスを指定できるようにしとく
	//MAGICK_TEMPORARY_PATH←これ
	//http://www.imagemagick.org/script/resources.php#environment
	return (0);
}

static enum vfp_hk_status __match_proto__(vfp_hk_pull_f)
	VFP_HK_ReadFullBody(struct vfp_ctx *vc, struct vfp_entry *vfe, void *priv,void **body, ssize_t *len){
	
	
	test(body,len,vc->bo,(struct vmod_poc_xcir_poc_xcir *)priv);

	VSLb(vc->bo->vsl, SLT_Debug, "VFP:vmod_vfp_pull_f:hoge");
	VSL_Flush(vc->bo->vsl,0);
	VSLb(vc->bo->vsl, SLT_Debug, "VFP:vmod_vfp_pull_f:ZZ / %ld,%s",*len,(char*)*body);
		
		
	//memcpy(*body,"zxz",2);
	//*len =2;
	return (VFP_HK_UPDATE);
}
static enum vfp_status __match_proto__(vfp_pull_f)
vmod_vfp_pull_f(struct vfp_ctx *vc, struct vfp_entry *vfe, void *p, ssize_t *lp){
	
	return vmod_vfp_wrap_pull_f(vc,vfe,p,lp);
}

static enum vfp_status __match_proto__(vfp_init_f)
vfp_pull_init(struct vfp_ctx *vc, struct vfp_entry *vfe)
{
	struct vfp_hk *vfp_hk_PREF;
	ALLOC_OBJ(vfp_hk_PREF, VFP_HK_MAGIC);
	vfp_hk_PREF->fullbody = VFP_HK_ReadFullBody;
	vfp_hk_PREF->bufsz    = 500*1024;//500KB
	vfp_hk_PREF->extendsz = 100*1024;//100KB
	vfp_hk_PREF->buffer   = calloc(vfp_hk_PREF->bufsz,1);

	vfp_hk_PREF->priv = alloc_vmod_poc_xcir_poc_xcir();
	
	vfe->priv1 = vfp_hk_PREF;
	
	
	return (VFP_OK);
}

static void __match_proto__(vfp_fini_f)
vfp_pull_fini(struct vfp_ctx *vc, struct vfp_entry *vfe)
{
	struct vfp_hk *vh = vfe->priv1;
	free_vmod_poc_xcir_poc_xcir(vh->priv);
	FREE_OBJ(vh);
	syslog(6,"fini");
}



struct vfp vfp_PREF = {
	.name = "TEST",
	.init = vfp_pull_init,
	.pull = vmod_vfp_pull_f,
	.fini = vfp_pull_fini,
};



	
	
VCL_VOID vmod_HookFetch(const struct vrt_ctx *ctx){
	(void)VFP_Push(ctx->bo->vfc,&vfp_PREF,1);
}



//////////////////////////
VCL_VOID vmod_imagickini(const struct vrt_ctx *ctx){
	MagickWandGenesis();
}
VCL_VOID vmod_imagickfini(const struct vrt_ctx *ctx){
	MagickWandTerminus();
}

