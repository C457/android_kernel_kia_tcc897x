#define QMI_WDA_SET_DATA_FORMAT		0x0020
#define QMI_WDA_SET_QMAP_SETTINGS	0x002B

struct qmi_wda_qos_data_format {
	u8 type;
	u16 length;
	u8 qos_format;
}__attribute__ ((packed));

struct qmi_wda_link_layer_protocol {
	u8 type;
	u16 length;
	u32 link_prot;
}__attribute__ ((packed));

struct qmi_wda_data_aggregation_protocol {
	u8 type;
	u16 length;
	u32 data_protocol;
}__attribute__ ((packed));

struct qmi_wda_data_aggregation_max_datagrams {
	u8 type;
	u16 length;
	u32 max_datagrams;
}__attribute__ ((packed));

struct qmi_wda_data_aggregation_max_size {
	u8 type;
	u16 length;
	u32 max_size;
}__attribute__ ((packed));

struct qmi_wda_peripheral_endpoint_type {
	u8 type;
	u16 length;
	u32 ep_type;
	u32 iface_id;
}__attribute__ ((packed));

struct qmi_wda_set_data_format_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_svc_sdu_header sdu_header;

	u16 message_id;
	u16 length;
}__attribute__ ((packed));

struct qmi_wda_in_band_flow_control {
	u8 type;
	u16 length;
	u8 flow_control;
}__attribute__ ((packed));

struct qmi_wda_set_qmap_settings_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_svc_sdu_header sdu_header;

	u16 message_id;
	u16 length;
}__attribute__ ((packed));
