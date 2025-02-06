#define QMI_SVC_IOCTL_BASE		  0x8BE0
#define QMI_SVC_IOCTL_SET_SERVICE QMI_SVC_IOCTL_BASE+1

#define QMI_SVC_IOCTL_SELECT_ADAPTER QMI_SVC_IOCTL_BASE+0x10

#define WAIT_NOTIFY_STATUS_NONE 0
#define WAIT_NOTIFY_STATUS_REMOVE 1
#define WAIT_NOTIFY_STATUS_CLOSE 2

#define QMI_SVC_MIN_TXID	0x0001
#define QMI_SVC_MAX_TXID	0xFFFF

struct qmi_svc_data_list {
	struct list_entry list;
	void *buffer;
	int length;
}__attribute__ ((packed));

struct qmi_svc_context {
	struct list_entry list;
	struct rmnet_context *parent_context;
	u8 service_type;
	u8 client_id;
	bool select_adapter;

	struct list_entry data_queue;

	int read_wait_active;
	void *read_wait_queue;
	void *buffer;
	int error_no;

	int notify_status;
	int wait_notify_active;
	void *wait_notify_queue;
	u8 real_dev_idx;
	u8 mux_dev_idx;
	atomic_t ref_cnt;
}__attribute__ ((packed));

struct qmi_svc_common_tlv {
	u8 type;
	u16 length;
	u8 value;
}__attribute__ ((packed));

struct qmi_svc_sdu_header {
	u8 control_flags;
	u16 transaction_id;
}__attribute__ ((packed));

struct qmi_svc_response_header {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_svc_sdu_header sdu_header;

	u16 message_id;
	u16 length;

	u8 tlv_type;
	u16 tlv_length;
	u16 qmi_result;
	u16 qmi_error;
}__attribute__ ((packed));

ssize_t qmi_svc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t qmi_svc_write(struct file *filp, const char *buf, size_t count, loff_t *fpos);
long qmi_svc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int qmi_svc_open(struct inode *inode, struct file *filp);
int qmi_svc_flush(struct file *filp, fl_owner_t id);
int qmi_svc_release(struct inode *inode, struct file *filp);
int qmi_svc_proc_response(struct rmnet_context *context, void *buffer, int length);

int qmi_svc_get_txid(int *txid);
int qmi_wda_set_data_format(struct rmnet_context *context);
int qmi_wda_set_qmap_settings(struct rmnet_context *context);
int qmi_wds_bind_mux_data_port(struct rmnet_context *context, u16 transaction_id, u8 client_id, u8 bind_mux_id);
int qmi_wds_set_client_ip_family(struct rmnet_context *context, u16 transaction_id, u8 client_id, u8 ip_pref);
int qmi_wds_indication_register(struct rmnet_context *context, u16 transaction_id, u8 client_id);
int qmi_wds_set_event_report(struct rmnet_context *context, u16 transaction_id, u8 client_id);
int qmi_wds_get_runtime_settings(struct rmnet_context *context, u16 transaction_id, u8 client_id);
int qmi_wds_get_current_channel_rate(struct rmnet_context *context, u16 transaction_id, u8 client_id);
int qmi_wds_get_pkt_srvc_status(struct rmnet_context *context, u8 ip_family);
int qmi_wds_bind_subscription(struct rmnet_context *context, u16 transaction_id, u8 client_id, u32 subscription);
int qmi_wds_start_network_interface(struct rmnet_context *context, u16 transaction_id, u8 client_id, u8 technology_preference, u8 profile_index);