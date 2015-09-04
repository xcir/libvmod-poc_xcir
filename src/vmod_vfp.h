

//////////////////
enum vfp_hk_status {
	VFP_HK_OK,
	VFP_HK_UPDATE,
};


typedef enum vfp_hk_status
    vfp_hk_pull_f(struct vfp_ctx *, struct vfp_entry *, void*priv, void **body, ssize_t *len);

typedef enum vfp_hk_status
    vfp_hk_error_f(struct vfp_ctx *, struct vfp_entry *);
    
struct vfp_hk {
	unsigned		magic;
#define VFP_HK_MAGIC		0x1e0fc9d9
	ssize_t szlim;
	vfp_hk_pull_f	*fullbody;
	vfp_hk_error_f	*fullbody_sizeover;
	vfp_hk_pull_f	*stream_before;
	vfp_hk_pull_f	*stream_after;
	void			*priv;
	
	
	
	ssize_t			offset_read;
	ssize_t			offset_write;
	int				read_done;
	void			*buffer;
	ssize_t			bufsz;
	ssize_t			extendsz;
	ssize_t			limit;
};
//////////////////
 enum vfp_status __match_proto__(vfp_pull_f)
vmod_vfp_wrap_pull_f(struct vfp_ctx *vc, struct vfp_entry *vfe, void *, ssize_t *);