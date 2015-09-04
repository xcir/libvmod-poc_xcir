#include <stdio.h>
#include <stdlib.h>

#include "vrt.h"
#include "cache/cache.h"

#include "vcc_if.h"

#include "cache/cache_filter.h"
#include "cache/cache_director.h"

#include "vmod_smalllight_param.h"
#include <syslog.h>


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

const char *vmod_smalllight_param_readParamRaw(struct busyobj *bo,const char* key){
	const char *p;
	http_GetHdrField(bo->bereq0, VMOD_PARAM_HEADER, key, &p);
	return p;
}
	
	
	
int vmod_smalllight_param_parse_color(struct busyobj *bo,const char* key, struct vmod_http_small_light_color_t *color)
{
	const char *p;
	char *sp;
	int len;
	p = vmod_smalllight_param_readParamRaw(bo, key);
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
unsigned vmod_smalllight_param_parse_bool(struct busyobj *bo,const char* key, char yes, unsigned def){
	const char *p;
	p = vmod_smalllight_param_readParamRaw(bo, key);
	if(p==NULL || p[0]=='\0'){
		return def;
	}
	if(p[0]==yes){
		return 1;
	}
	return def;
	
}
double vmod_smalllight_param_parse_double(struct busyobj *bo,const char* key){
	const char *p;
	p = vmod_smalllight_param_readParamRaw(bo, key);
	if(p==NULL) return 0;
	return atof(p);
}
int vmod_smalllight_param_parse_coord(struct busyobj *bo,const char* key, struct vmod_http_small_light_coord_t *d){
	char *er;
	const char *p;
	p = vmod_smalllight_param_readParamRaw(bo, key);
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






void vmod_smalllight_param_get_val_txt(struct busyobj *bo, const char* key, char *buffer, int conma, int sz){
	//カンマ分スキップする（blur=radius,sigma用）
	const char *p;
	char *sp=NULL;
	int i,len;
	p = vmod_smalllight_param_readParamRaw(bo, key);
	if(p == NULL){
		buffer[0]='\0';
		return;
	}
	sp = (char*)p;
	for(i=0;i < conma;i++){
		sp = strstr(sp + i,",");
		if(sp == NULL){
			sp = (char*)p + strlen(p);
			break;
		}
	}
	len = sp - (char*)p;
	if(i < conma -1 || sz < len){
		buffer[0]='\0';
		return;
	}
	strncpy(buffer,p,len);
	buffer[len] = '\0';
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

void readParam(struct busyobj *bo, struct vmod_poc_xcir_poc_xcir* pr){
	const char *p;
	char bf[32];
	
	vmod_smalllight_param_parse_coord(bo,"sx",pr->sx);
	vmod_smalllight_param_parse_coord(bo,"sy",pr->sy);
	vmod_smalllight_param_parse_coord(bo,"sw",pr->sw);
	vmod_smalllight_param_parse_coord(bo,"sh",pr->sh);

	vmod_smalllight_param_parse_coord(bo,"dx",pr->dx);
	vmod_smalllight_param_parse_coord(bo,"dy",pr->dy);
	vmod_smalllight_param_parse_coord(bo,"dw",pr->dw);
	vmod_smalllight_param_parse_coord(bo,"dh",pr->dh);

	p = vmod_smalllight_param_readParamRaw(bo, "da");
	if(p==NULL || p[0]=='l'){
		pr->da = VMOD_HTTP_SMALL_LIGHT_DA_LONG_EDGE;
	}else if(p[0]=='s'){
		pr->da = VMOD_HTTP_SMALL_LIGHT_DA_SHORT_EDGE;
	}else if(p[0]=='n'){
		pr->da = VMOD_HTTP_SMALL_LIGHT_DA_NOPE;
	}else{
		pr->da = VMOD_HTTP_SMALL_LIGHT_DA_LONG_EDGE;
	}

	p = vmod_smalllight_param_readParamRaw(bo, "ds");
	if(p==NULL || p[0]=='n'){
		pr->ds = VMOD_HTTP_SMALL_LIGHT_DS_NO_SCALE_SMALL_IMAGE;
	}else if(p[0]=='f'){
		pr->ds = VMOD_HTTP_SMALL_LIGHT_DS_FORCE_SCALE;
	}else{
		pr->ds = VMOD_HTTP_SMALL_LIGHT_DS_NO_SCALE_SMALL_IMAGE;
	}
	
	pr->cw = vmod_smalllight_param_parse_double(bo,"cw");
	pr->ch = vmod_smalllight_param_parse_double(bo,"ch");
	vmod_smalllight_param_parse_color(bo,"cc",pr->cc);

	pr->bw = vmod_smalllight_param_parse_double(bo,"bw");
	pr->bh = vmod_smalllight_param_parse_double(bo,"bh");
	vmod_smalllight_param_parse_color(bo,"bc",pr->bc);
	

	vmod_smalllight_param_get_val_txt(bo,"pt",bf,1,sizeof(bf));
	if(bf==NULL || bf[0]=='n'){
		pr->pt = VMOD_HTTP_SMALL_LIGHT_PT_NOPE;
	}else if(0==strcmp(bf,"ptss")){
		pr->pt = VMOD_HTTP_SMALL_LIGHT_PT_PTSS;
	}else if(0==strcmp(bf,"ptls")){
		pr->pt = VMOD_HTTP_SMALL_LIGHT_PT_PTLS;
	}else{
		pr->pt = VMOD_HTTP_SMALL_LIGHT_PT_NOPE;
	}

	//未指定の時の動きを作る（q維持）
	pr->q = vmod_smalllight_param_parse_double(bo,"q");
	if(pr->q < 0){
		pr->q=0;
	}else if(pr->q > 100){
		pr->q=100;
	}
	
	vmod_smalllight_param_get_val_txt(bo,"of",bf,1,sizeof(bf));
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

	pr->inhexif  = vmod_smalllight_param_parse_bool(bo,"inhexif"  ,'y',0);
	pr->jpeghint = vmod_smalllight_param_parse_bool(bo,"jpeghint" ,'y',0);
	pr->info     = vmod_smalllight_param_parse_bool(bo,"info"     ,'1',0);
	//void vmod_smalllight_param_get_val_txt(struct busyobj *bo,const char* key,char *buffer,int conma,int sz){
	vmod_smalllight_param_get_val_txt(bo,"e",bf,1,sizeof(bf));
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
	p = vmod_smalllight_param_readParamRaw(bo, "e");
	*/

}
