#define QMI_CTL_SET_INSTANCE_ID 			0x0020
#define QMI_CTL_GET_VERSION_INFO 			0x0021
#define QMI_CTL_GET_CLIENT_ID 				0x0022
#define QMI_CTL_RELEASE_CLIENT_ID 			0x0023
#define QMI_CTL_REVOKE_CLIENT_ID_IND 		0x0024
#define QMI_CTL_INVALID_CLIENT_ID_IND 		0x0025
#define QMI_CTL_SET_DATA_FORMAT 			0x0026
#define QMI_CTL_SYNC 						0x0027
#define QMI_CTL_SET_EVENT_REPORT 			0x0028
#define QMI_CTL_CONFIG_PWR_SAVE_SETTINGS 	0x0029
#define QMI_CTL_SET_PWR_SAVE_MODE 			0x002A
#define QMI_CTL_GET_PWR_SAVE_MODE 			0x002B

#define QMI_CTL_MIN_TXID	0x01
#define QMI_CTL_MAX_TXID	0xff

struct qmi_ctl_transaction_item {
	struct list_entry list;
	void *wait_queue_head;
	u8 service_type;
	u8 client_id;
	u8 transaction_id;
	void *buffer;
	int length;
	u16 qmi_result;
	u16 qmi_error;
}__attribute__ ((packed));

struct qmi_ctl_send_msg_context {
	void *wait_queue_head;
	int msg_length;
}__attribute__ ((packed));

struct qmi_qmux_msg_header {
	u8 if_type;
	u16 length;
	u8 control_flags;
	u8 service_type;
	u8 client_id;
}__attribute__ ((packed));

#define QMUX_SDU_MSG_OFFSET(qmux) ((void*)(((struct qmi_qmux_msg_header *)qmux) + 1))
#define QMUX_SDU_MSG_SIZE(qmux) ((qmux)->length - sizeof(struct qmi_qmux_msg_header) + 1)

struct qmi_ctl_sdu_header {
	u8 control_flags;
	u8 transaction_id;
}__attribute__ ((packed));

struct qmi_ctl_response_header {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_ctl_sdu_header sdu_header;

	u16 message_id;
	u16 length;

	u8 tlv_type;
	u16 tlv_length;
	u16 qmi_result;
	u16 qmi_error;
}__attribute__ ((packed));

struct qmi_ctl_get_version_info_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_ctl_sdu_header sdu_header;

	u16 message_id;
	u16 length;
}__attribute__ ((packed));

struct qmi_ctl_qmux_version_info {
	u8 qmi_svc_type;
	u16 major_ver;
	u16 minor_ver;
}__attribute__ ((packed));

struct qmi_ctl_qmux_version_list {
	u8 type;
	u16 length;
	u8 num_instances;
}__attribute__ ((packed));

struct qmi_ctl_get_client_id_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_ctl_sdu_header sdu_header;

	u16 message_id;
	u16 length;

	u8 tlv_type;
	u16 tlv_length;
	u8 qmi_svc_type;
}__attribute__ ((packed));

struct qmi_ctl_assigned_client_id {
	u8 type;
	u16 length;
	u8 qmi_svc_type;
	u8 client_id;
}__attribute__ ((packed));

struct qmi_ctl_release_client_id_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_ctl_sdu_header sdu_header;

	u16 message_id;
	u16 length;

	u8 tlv_type;
	u16 tlv_length;
	u8 qmi_svc_type;
	u8 client_id;
}__attribute__ ((packed));

struct qmi_ctl_sync_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_ctl_sdu_header sdu_header;

	u16 message_id;
	u16 length;
}__attribute__ ((packed));

struct qmi_ctl_host_driver_instance {
	u8 type;
	u16 length;
	u8 host_driver_instance;
}__attribute__((packed));

struct qmi_ctl_set_instance_id_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_ctl_sdu_header sdu_header;

	u16 message_id;
	u16 length;
	
	struct qmi_ctl_host_driver_instance host_driver_instance;
}__attribute__((packed));

int qmi_ctl_get_version_info(struct rmnet_context *context);
int qmi_ctl_get_client_id(struct rmnet_context *context, u8 service_type, u8 *client_id);
int qmi_ctl_release_client_id(struct rmnet_context *context, u8 service_type, u8 client_id);
int qmi_ctl_sync(struct rmnet_context *context);
int qmi_ctl_set_instance_id(struct rmnet_context *context);
int qmi_ctl_proc_response(struct rmnet_context *context, void *buffer, int length);

