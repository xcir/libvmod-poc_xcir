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

/*
	リスト向けのメモ(agif)
	MagickResetIterator(wand);
	while (MagickNextImage(wand) != MagickFalse){
		MagickResizeImage(wand, pr->dw->v, pr->dh->v ,LanczosFilter,1.0);
	}
*/
	
	
void test(void** body,ssize_t *sz, struct busyobj *bo, struct vmod_smalllight_param* pr){
	

	
	char bf[32];
	char *org_f;
	MagickWand	*wand;
	ColorspaceType   color_space;
	//MagickWandGenesis();
	
	wand = NewMagickWand();
	
	vmod_smalllight_param_read(bo,pr);
	
	//jpeg hint
	if(pr->jpeghint){
		snprintf(bf,32,"%dx%d",(int)pr->dw->v,(int)pr->dh->v);
		MagickSetOption(wand, "jpeg:size", bf);
	}
	
	MagickReadImageBlob(wand, *body, *sz);
	pr->iw = (double)MagickGetImageWidth(wand);
	pr->ih = (double)MagickGetImageHeight(wand);
	vmod_smalllight_param_calc(bo, pr);
	if(pr->f_pt){
		DestroyMagickWand(wand);
		return;
	}
	
	
	color_space = MagickGetImageColorspace(wand);
	org_f = MagickGetImageFormat(wand);
	//free
	free(*body);
	vmod_smalllight_param_calc(bo, pr);

	unsigned long nim =  MagickGetNumberImages(wand);
	if(nim > 1){
		//agif
		//todo
	}else{
		//通常画像
		//todo
	}
	
	
	
	//crop
	if(pr->f_crop){
		VSLb(bo->vsl, SLT_Debug, "IMAGICK:MagickCropImage:%f,%f,%f,%f",pr->sw->v, pr->sh->v,pr->sx->v, pr->sy->v);
		MagickCropImage(wand,
			pr->sw->v, pr->sh->v,
			pr->sx->v, pr->sy->v
		);
	}
	//scale
	if(pr->f_scale){
		VSLb(bo->vsl, SLT_Debug, "IMAGICK:MagickResizeImage:%f,%f",pr->dw->v, pr->dh->v);
		MagickResizeImage(wand, pr->dw->v, pr->dh->v ,LanczosFilter,1.0);
	}
	//canvas
	MagickWand    *canvas_wand;
	PixelWand     *canvas_color;

	if      (pr->dx->v > 0 || pr->dy->v > 0){
		//EXTENT
		VSLb(bo->vsl, SLT_Debug, "EXTENT:A");
		canvas_wand  = NewMagickWand();
		//これテンポラリあとでなんとかする
		MagickSetFormat(canvas_wand, org_f);
		//
		canvas_color = NewPixelWand();
		VSLb(bo->vsl, SLT_Debug, "EXTENT:E");
		PixelSetRed(canvas_color,   pr->cc->r / 255.0);
		PixelSetGreen(canvas_color, pr->cc->g / 255.0);
		PixelSetBlue(canvas_color,  pr->cc->b / 255.0);
		PixelSetAlpha(canvas_color, pr->cc->a / 255.0);
		VSLb(bo->vsl, SLT_Debug, "EXTENT:F %f %f %f %f",pr->cw, pr->ch,pr->dx->v, pr->dy->v);
		VSL_Flush(bo->vsl,0);
		MagickNewImage(canvas_wand, pr->cw, pr->ch, canvas_color);
		VSLb(bo->vsl, SLT_Debug, "EXTENT:G");
		VSL_Flush(bo->vsl,0);
		DestroyPixelWand(canvas_color);
		MagickTransformImageColorspace(canvas_wand, color_space);
		MagickCompositeImage(canvas_wand, wand, AtopCompositeOp, pr->dx->v, pr->dy->v);
		DestroyMagickWand(wand);
		wand = canvas_wand;
	}else if(pr->dw->v == pr->cw && pr->dh->v == pr->ch){
		VSLb(bo->vsl, SLT_Debug, "EXTENT:B");
		//nowork
		
	}else if(pr->dw->v > pr->cw && pr->dh->v > pr->ch){
		//crop
		VSLb(bo->vsl, SLT_Debug, "EXTENT:C");
		MagickCropImage(wand,
			pr->cw,pr->ch,
			0,0
		);
	}else{
		//extent
		
		VSLb(bo->vsl, SLT_Debug, "EXTENT:D");
		canvas_wand  = NewMagickWand();
		//これテンポラリあとでなんとかする
		MagickSetFormat(canvas_wand, org_f);
		//
		canvas_color = NewPixelWand();
		VSLb(bo->vsl, SLT_Debug, "EXTENT:E");
		PixelSetRed(canvas_color,   pr->cc->r / 255.0);
		PixelSetGreen(canvas_color, pr->cc->g / 255.0);
		PixelSetBlue(canvas_color,  pr->cc->b / 255.0);
		PixelSetAlpha(canvas_color, pr->cc->a / 255.0);
		VSLb(bo->vsl, SLT_Debug, "EXTENT:F %f %f %f %f",pr->cw, pr->ch,pr->dx->v, pr->dy->v);
		MagickNewImage(canvas_wand, pr->cw, pr->ch, canvas_color);
		DestroyPixelWand(canvas_color);
		MagickTransformImageColorspace(canvas_wand, color_space);
		MagickCompositeImage(canvas_wand, wand, AtopCompositeOp, pr->dx->v, pr->dy->v);
		DestroyMagickWand(wand);
		wand = canvas_wand;
		
	}
	
	
	if(pr->q != VMOD_SMALLLIGHT_PARAM_IGNORE_Q){
		VSLb(bo->vsl, SLT_Debug, "MagickSetImageCompressionQuality %f",pr->q);
		MagickSetImageCompressionQuality(wand, pr->q);
	}
	
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

