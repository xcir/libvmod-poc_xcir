#include <stdio.h>
#include <stdlib.h>

#include "vrt.h"
#include "cache/cache.h"

#include "vcc_if.h"

#include "cache/cache_filter.h"
#include "cache/cache_director.h"

#include <syslog.h>
#include "vmod_vfp.h"
#include <wand/MagickWand.h>
#include "vmod_smalllight_param.h"


/*
Known problem
	- Should unset beresp.http.content-length
	- Should set the thread_pool_stack=512k in runtime-param

todo:
	- x-vmod-smalllight-paramで固定のしか割り当てられないので-p相当の機能を実施するために-before-paramを作成してそっちを先にパースして上書きする仕組みを作る



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


/*
	
	[clthread]req.http.x-vmod-smalllight-param
	[bgthread][vcl_backend_fetch]bereq0.http.x-vmod-smalllight-param -(copy)->bereq.http.x-vmod-smalllight-param ->REMOVE
	[bgthread] set to vfp
	[VFP] read from bereq0.http.x-vmod-smalllight-param
	
	
*/


void test(void** body,ssize_t *sz, struct busyobj *bo, struct vmod_smalllight_param* pr){
	

	
	
	MagickWand	*wand;
	//MagickWandGenesis();
	
	wand = NewMagickWand();
	
	//dw,dhを指定
	
	MagickReadImageBlob(wand, *body, *sz);
	MagickResetIterator(wand);
	
	//syslog(6,"%f",smlp->sx->v); 
	pr->iw = (double)MagickGetImageWidth(wand);
	pr->ih = (double)MagickGetImageWidth(wand);
	//prcalc(pr);
	
	//if(pr->sx < iw)
	vmod_smalllight_param_read(bo,pr);
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
	
	
	test(body,len,vc->bo,(struct vmod_smalllight_param *)priv);

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

	vfp_hk_PREF->priv = vmod_smalllight_param_alloc();
	
	vfe->priv1 = vfp_hk_PREF;
	
	
	return (VFP_OK);
}

static void __match_proto__(vfp_fini_f)
vfp_pull_fini(struct vfp_ctx *vc, struct vfp_entry *vfe)
{
	struct vfp_hk *vh = vfe->priv1;
	vmod_smalllight_param_free(vh->priv);
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

