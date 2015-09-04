#include <syslog.h>
#include "vmod_poc_vfp.h"
#include <wand/MagickWand.h>
#include "vmod_poc_param.h"


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

const char *readParamRaw(struct busyobj *bo,const char* key){
	const char *p;
	http_GetHdrField(bo->bereq0, VMOD_PARAM_HEADER, key, &p);
	return p;
}
	
	
	
int parse_color(struct busyobj *bo,const char* key, struct vmod_http_small_light_color_t *color)
{
	const char *p;
	char *sp;
	int len;
	p = readParamRaw(bo, key);
	if(p==NULL) return 0;
	
	sp = strstr(p, ",");
	if(sp == NULL){
		len = strlen(p);
	}else{
		len = sp - p;
	}

	int res;
    if (len == 3) {
        res = sscanf(p, "%1hx%1hx%1hx", &color->r, &color->g, &color->b);
        if (res != EOF) {
            color->a = 255;
            return 1;
        }
    } else if (len == 4) {
        res = sscanf(p, "%1hx%1hx%1hx%1hx", &color->r, &color->g, &color->b, &color->a);
        if (res != EOF) {
            return 1;
        }
    } else if (len == 6) {
        res = sscanf(p, "%02hx%02hx%02hx", &color->r, &color->g, &color->b);
        if (res != EOF) {
            color->a = 255;
            return 1;
        }
    } else if (len == 8) {
        res = sscanf(p, "%02hx%02hx%02hx%02hx", &color->r, &color->g, &color->b, &color->a);
        if (res != EOF) {
            return 1;
        }
    }
    return 0;
}
unsigned parse_bool(struct busyobj *bo,const char* key, char yes, unsigned def){
	const char *p;
	p = readParamRaw(bo, key);
	if(p==NULL || p[0]=='\0'){
		return def;
	}
	if(p[0]==yes){
		return 1;
	}
	return def;
	
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
	///////////////////
	ALLOC_OBJ(sml->cc, VMOD_HTTP_SMALL_LIGHT_COLOR_T_MAGIC);
	AN(sml->cc);
	///////////////////
	ALLOC_OBJ(sml->bc, VMOD_HTTP_SMALL_LIGHT_COLOR_T_MAGIC);
	AN(sml->bc);
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
	CHECK_OBJ_NOTNULL(sml->cc, VMOD_HTTP_SMALL_LIGHT_COLOR_T_MAGIC);
	FREE_OBJ(sml->cc);
	////////////////////////
	CHECK_OBJ_NOTNULL(sml->bc, VMOD_HTTP_SMALL_LIGHT_COLOR_T_MAGIC);
	FREE_OBJ(sml->bc);

	CHECK_OBJ_NOTNULL(sml, VMOD_POC_XCIR_POC_XCIR_MAGIC);
	FREE_OBJ(sml);

}
VCL_VOID vmod_poc_xcir__init(const struct vrt_ctx *ctx, struct vmod_poc_xcir_poc_xcir **smlp, const char *vcl_name
	){

}
VCL_VOID vmod_poc_xcir__fini(struct vmod_poc_xcir_poc_xcir **smlp)
{

}

void getVal(struct busyobj *bo,const char* key,char *buffer,int conma,int sz){
	//カンマ分スキップする（blur=radius,sigma用）
	const char *p;
	char *sp=NULL;
	int i,len;
	p = readParamRaw(bo, key);
	if(p == NULL){
		buffer[0]='\0';
		return;
	}
	syslog(6,"aa:%s",p);
	sp = (char*)p;
	//int len = strlen(p);
	for(i=0;i < conma;i++){
		sp = strstr(sp + i,",");
		if(sp == NULL){
			sp = (char*)p + strlen(p);
			break;
		}
	}
	len = sp - (char*)p;
	syslog(6,"aab:%d %d %d",len,i,sz);
	if(i < conma -1 || sz < len){
		buffer[0]='\0';
		return;
	}
	strncpy(buffer,p,len);
	buffer[len] = '\0';
}
void readParam(struct busyobj *bo, struct vmod_poc_xcir_poc_xcir* pr){
	const char *p;
	char bf[32];
	
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
		pr->da = VMOD_HTTP_SMALL_LIGHT_DA_LONG_EDGE;
	}else if(p[0]=='s'){
		pr->da = VMOD_HTTP_SMALL_LIGHT_DA_SHORT_EDGE;
	}else if(p[0]=='n'){
		pr->da = VMOD_HTTP_SMALL_LIGHT_DA_NOPE;
	}else{
		pr->da = VMOD_HTTP_SMALL_LIGHT_DA_LONG_EDGE;
	}

	p = readParamRaw(bo, "ds");
	if(p==NULL || p[0]=='n'){
		pr->ds = VMOD_HTTP_SMALL_LIGHT_DS_NO_SCALE_SMALL_IMAGE;
	}else if(p[0]=='f'){
		pr->ds = VMOD_HTTP_SMALL_LIGHT_DS_FORCE_SCALE;
	}else{
		pr->ds = VMOD_HTTP_SMALL_LIGHT_DS_NO_SCALE_SMALL_IMAGE;
	}
	
	pr->cw = parse_double(bo,"cw");
	pr->ch = parse_double(bo,"ch");
	parse_color(bo,"cc",pr->cc);

	pr->bw = parse_double(bo,"bw");
	pr->bh = parse_double(bo,"bh");
	parse_color(bo,"bc",pr->bc);
	

	getVal(bo,"pt",bf,1,sizeof(bf));
	if(bf==NULL || bf[0]=='n'){
		pr->pt = VMOD_HTTP_SMALL_LIGHT_PT_NOPE;
	}else if(0==strcmp(bf,"ptss")){
		pr->pt = VMOD_HTTP_SMALL_LIGHT_PT_PTSS;
	}else if(0==strcmp(bf,"ptls")){
		pr->pt = VMOD_HTTP_SMALL_LIGHT_PT_PTLS;
	}else{
		pr->pt = VMOD_HTTP_SMALL_LIGHT_PT_NOPE;
	}

	pr->q = parse_double(bo,"q");
	if(pr->q < 0){
		pr->q=0;
	}else if(pr->q > 100){
		pr->q=100;
	}
	
	getVal(bo,"of",bf,1,sizeof(bf));
	if(bf==NULL){
		pr->of = VMOD_HTTP_SMALL_LIGHT_OF_AUTO;
	}else if(0==strcmp(bf,"png")){
		pr->of = VMOD_HTTP_SMALL_LIGHT_OF_PNG;
	}else if(0==strcmp(bf,"gif")){
		pr->of = VMOD_HTTP_SMALL_LIGHT_OF_GIF;
	}else if(0==strcmp(bf,"jpeg")){
		pr->of = VMOD_HTTP_SMALL_LIGHT_OF_JPEG;
	}else if(0==strcmp(bf,"tiff")){
		pr->of = VMOD_HTTP_SMALL_LIGHT_OF_TIFF;
	}else{
		pr->of = VMOD_HTTP_SMALL_LIGHT_OF_AUTO;
	}

	pr->inhexif  = parse_bool(bo,"inhexif"  ,'y',0);
	pr->jpeghint = parse_bool(bo,"jpeghint" ,'y',0);
	pr->info     = parse_bool(bo,"info"     ,'1',0);
	//void getVal(struct busyobj *bo,const char* key,char *buffer,int conma,int sz){
	getVal(bo,"e",bf,1,sizeof(bf));
	if(bf==NULL){
		pr->e = VMOD_HTTP_SMALL_LIGHT_E_DUMMY;
//	}else if(0==strcmp(bf,"imlib2")){
//		pr->e = VMOD_HTTP_SMALL_LIGHT_E_IMLIB2;
	}else if(0==strcmp(bf,"imagemagick")){
		pr->e = VMOD_HTTP_SMALL_LIGHT_E_IMAGEMAGICK;
	}else{
		pr->e = VMOD_HTTP_SMALL_LIGHT_E_DUMMY;
	}

	/*
	char[10]ぐらいの定義してそこにコピーして評価したほうがいいね
	p = readParamRaw(bo, "e");
	*/

}

void test(void** body,ssize_t *sz, struct busyobj *bo, struct vmod_poc_xcir_poc_xcir* pr){
	

	
	
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

