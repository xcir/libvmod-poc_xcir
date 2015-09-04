#include <stdio.h>
#include <stdlib.h>

#include "vrt.h"
#include "cache/cache.h"

#include "vcc_if.h"

#include "cache/cache_filter.h"
#include "cache/cache_director.h"

#include "vmod_vfp.h"


enum vfp_status __match_proto__(vfp_pull_f)
vmod_vfp_wrap_pull_f(struct vfp_ctx *vc, struct vfp_entry *vfe, void *p, ssize_t *lp){
	VSLb(vc->bo->vsl, SLT_Debug, "VFP:vmod_vfp_pull_f:firefire2 %ld",vc->bo->htc->content_length);

	//確保されてるストレージのサイズ
	ssize_t st_size = *lp;
	
	VSLb(vc->bo->vsl, SLT_Debug, "VFP:vmod_vfp_pull_f:START lx=%ld",*lp);
	enum vfp_status vp;
	
	struct vfp_hk *vh = (struct vfp_hk *)vfe->priv1;
	
	CHECK_OBJ_NOTNULL(vc, VFP_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(vfe, VFP_ENTRY_MAGIC);
	
	/////////////////////////
	/////////////////////////
	vp = VFP_Suck(vc, p, lp);
	/////////////////////////
	/////////////////////////
	
	//get full set data
	if(vh->fullbody){
		if(vh->read_done){
			vp = VFP_OK;
			//最大のcpsizeはストレージサイズ
			ssize_t cps = st_size;
			//サイズチェック
			if(vh->bufsz - vh->offset_read < cps){
				void *tmp = realloc(vh->buffer,vh->bufsz + vh->extendsz);
				if(tmp == NULL){
					free(vh->buffer);
					AN(tmp);
				}
				vh->bufsz += vh->extendsz;
				VSLb(vc->bo->vsl, SLT_Debug, "VFP:vmod_vfp_pull_f:EXTEND %ld",vh->bufsz);
				vh->buffer = tmp;
			}
			if(vh->offset_read - vh->offset_write < st_size){
				cps = vh->offset_read - vh->offset_write;
				vp = VFP_END;
			}
			memcpy(p,vh->buffer + vh->offset_write,cps);
			VSLb(vc->bo->vsl, SLT_Debug, "VFP:vmod_vfp_pull_f:SZZ %ld %ld %s",vh->offset_read,vh->offset_write,(char*)p);
			vh->offset_write+=cps;
			*lp = cps;
			if(vh->offset_read - vh->offset_write <= 0){
				free(vh->buffer);
				vp=VFP_END;
			}
		}else{
			if(*lp > 0){
				//バッファにコピーする
				memcpy(vh->buffer + vh->offset_read,p,*lp);
				vh->offset_read += *lp;
			}
			//extendされないように0にセット
			*lp=0;
			if(vp == VFP_END){
				//読み込みが完了
				vh->read_done=1;
				vp = VFP_OK;
				//ここでfullbody処理する
				void **body = &vh->buffer;
				enum vfp_hk_status vhs;
				VSLb(vc->bo->vsl, SLT_Debug, "VFP:vmod_vfp_pull_f:FULLBODY ");
				////
				vhs = vh->fullbody(vc,vfe,vh->priv,body,&vh->offset_read);
				////
				VSLb(vc->bo->vsl, SLT_Debug, "VFP:vmod_vfp_pull_f:FULLBODY:DONE %s ",(char*)vh->buffer);

				if(vhs == VFP_HK_UPDATE){
				}
			}
			
		}
	}
	VSL_Flush(vc->bo->vsl,0);
	return (vp);
}


