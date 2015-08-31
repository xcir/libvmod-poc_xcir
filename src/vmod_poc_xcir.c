#include <syslog.h>
#include "vmod_poc_vfp.h"
#include <wand/MagickWand.h>

/*
Known problem
	- Should unset beresp.http.content-length
	- Should set the thread_pool_stack=512k in runtime-param




*/
struct vmod_smalllight_buffer {
	unsigned				magic;
#define VMOD_SMALLLIGHT_BUFFER_MAGIC		0x8d4cef02
	void *imgbuf;
	size_t imglen;
	size_t imgcur;//現在のポインタ
};



void test(void** body,ssize_t *sz){
	//これ増やさないと落ちる・・・
	// thread_pool_stack=512k

	
	
//	syslog(6,"HELLO");
//	return(0);
	MagickWand	*wand;
	//MagickWandGenesis();
	
	wand = NewMagickWand();
	
	
	MagickReadImageBlob(wand, *body, *sz);
	MagickResetIterator(wand);
	while (MagickNextImage(wand) != MagickFalse){
		MagickResizeImage(wand,50,50,LanczosFilter,1.0);
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
	return (0);
}

static enum vfp_hk_status __match_proto__(vfp_hk_pull_f)
	VFP_HK_ReadFullBody(struct vfp_ctx *vc, struct vfp_entry *vfe, void *priv,void **body, ssize_t *len){
	
	
	test(body,len);
	//test(body,*len,vfe);//Imagemagickproc

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

	
	struct vmod_smalllight_buffer *sb;
	ALLOC_OBJ(sb, VMOD_SMALLLIGHT_BUFFER_MAGIC);
	vfp_hk_PREF->priv = sb;
	
	
	vfe->priv1 = vfp_hk_PREF;
	
	
	return (VFP_OK);
}

static void __match_proto__(vfp_fini_f)
vfp_pull_fini(struct vfp_ctx *vc, struct vfp_entry *vfe)
{
	struct vfp_hk *vh = vfe->priv1;
	struct vmod_smalllight_buffer *sb = (struct vmod_smalllight_buffer *)vh->priv;
	FREE_OBJ(sb);
	FREE_OBJ(vh);
	//if(vfe->priv1) vmod_smalllight_buffer_free(vfe->priv1);
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

