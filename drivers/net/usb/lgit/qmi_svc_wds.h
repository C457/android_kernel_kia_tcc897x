#define QMI_WDS_RESET							0x0000
#define QMI_WDS_SET_EVENT_REPORT				0x0001
#define QMI_WDS_EVENT_REPORT_IND				0x0001
#define QMI_WDS_ABORT							0x0002
#define QMI_WDS_INDICATION_REGISTER				0X0003
#define QMI_WDS_START_NETWORK_INTERFACE			0x0020
#define QMI_WDS_STOP_NETWORK_INTERFACE			0x0021
#define QMI_WDS_GET_PKT_SRVC_STATUS				0x0022
#define QMI_WDS_GET_PKT_SRVC_STATUS_IND			0x0022
#define QMI_WDS_GET_CURRENT_CHANNEL_RATE		0x0023
#define QMI_WDS_GET_PKT_STATISTICS				0x0024
#define QMI_WDS_GO_DORMANT						0x0025
#define QMI_WDS_GO_ACTIVE						0x0026
#define QMI_WDS_CREATE_PROFILE					0x0027
#define QMI_WDS_MODIFY_PROFILE_SETTINGS			0x0028
#define QMI_WDS_DELETE_PROFILE					0x0029
#define QMI_WDS_GET_PROFILE_LIST				0x002A
#define QMI_WDS_GET_PROFILE_SETTINGS			0x002B
#define QMI_WDS_GET_DEFAULT_SETTINGS			0x002C
#define QMI_WDS_GET_RUNTIME_SETTINGS			0x002D
#define QMI_WDS_SET_MIP_MODE					0x002E
#define QMI_WDS_GET_MIP_MODE					0x002F
#define QMI_WDS_GET_DORMANCY_STATUS				0x0030
#define QMI_WDS_GET_CALL_DURATION				0x0035
#define QMI_WDS_GET_CURRENT_DATA_BEARER_TECHNOLOGY	0x0044
#define QMI_WDS_SET_CLIENT_IP_FAMILY_PREF		0x004D
#define QMI_WDS_EXTENDED_IP_CONFIG_IND			0x008C
#define QMI_WDS_BIND_MUX_DATA_PORT				0x00A2
#define QMI_WDS_BIND_SUBSCRIPTION				0X00AF
#define QMI_WDS_START_NETWORK_INTERFACE			0x0020

#define QMI_WDS_IP_FAMILY_IPV4					0x04
#define QMI_WDS_IP_FAMILY_IPV6					0x06

struct qmi_wds_channel_rate_indicator {
	u8 type;
	u16 length;
	u8 report_indicator;
}__attribute__ ((packed));

struct qmi_wds_transfer_stat_indicator {
	u8 type;
	u16 length;
	u8 stats_period;
	u32 stats_mask;
}__attribute__ ((packed));

struct qmi_wds_current_bearer_tech_indicator {
	u8 type;
	u16 length;
	u8 report_bearer;
}__attribute__ ((packed));

struct qmi_wds_dormancy_stat_indicator {
	u8 type;
	u16 length;
	u8 report_dormancy;
}__attribute__ ((packed));

struct qmi_wds_set_event_report_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_svc_sdu_header sdu_header;

	u16 message_id;
	u16 length;

	struct qmi_wds_channel_rate_indicator channel_rate;
	struct qmi_wds_transfer_stat_indicator transfer_stat;
	struct qmi_wds_current_bearer_tech_indicator bearer_tech;
	struct qmi_wds_dormancy_stat_indicator dormancy_stat;
}__attribute__ ((packed));

struct qmi_wds_channel_rate {
	u8 type;
	u16 length;
	u32 tx_rate;
	u32 rx_rate;
}__attribute__ ((packed));

struct qmi_wds_connection_status {
	u8 type;
	u16 length;
	u8 connection_status;
}__attribute__ ((packed));

struct qmi_wds_packet_service_status {
	u8 type;
	u16 length;
	u8 connection_status;
	u8 reconfig_req;
}__attribute__ ((packed));

struct qmi_wds_call_end_reason {
	u8 type;
	u16 length;
	u16 call_end_reason;
}__attribute__ ((packed));

struct qmi_wds_verbose_call_end_reason {
	u8 type;
	u16 length;
	u16 call_end_reason_type;
	u16 call_end_reason;
}__attribute__ ((packed));

struct qmi_wds_ip_family {
	u8 type;
	u16 length;
	u8 ip_family;
}__attribute__ ((packed));

struct qmi_wds_peripheral_end_point_id {
	u8 type;
	u16 length;
	u32 ep_type;
	u32 iface_id;
}__attribute__((packed));

struct qmi_wds_mux_id {
	u8 type;
	u16 length;
	u8 mux_id;
}__attribute__((packed));

struct qmi_wds_client_type {
	u8 type;
	u16 length;
	u32 client_type;
}__attribute__((packed));

struct qmi_wds_mux_data_port_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_svc_sdu_header sdu_header;
	
	u16 message_id;
	u16 length;
}__attribute__((packed));

struct qmi_wds_ip_family_preference {
	u8 type;
	u16 length;
	u8 ip_preference;
}__attribute__((packed));

struct qmi_wds_set_client_ip_family_pref_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_svc_sdu_header sdu_header;
	
	u16 message_id;
	u16 length;
	
	struct qmi_wds_ip_family_preference ip_preference;
}__attribute__((packed));

struct qmi_wds_bind_subscription_tlv {
	u8 type;
	u16 length;
	u32 subscription;
}__attribute__((packed));

struct qmi_wds_bind_subscription_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_svc_sdu_header sdu_header;
	
	u16 message_id;
	u16 length;
	
	struct qmi_wds_bind_subscription_tlv subscription;
}__attribute__((packed));

struct qmi_wds_start_network_interface_tlv {
	u8 type1;
	u16 length1;
	u8 technology_preference;
	u8 type2;
	u16 length2;
	u8 profile_index;
}__attribute__((packed));

struct qmi_wds_start_network_interface_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_svc_sdu_header sdu_header;
	
	u16 message_id;
	u16 length;
	
	struct qmi_wds_start_network_interface_tlv tlvs;
}__attribute__((packed));

struct qmi_wds_extended_ip_configuration_change {
	u8 type;
	u16 length;
	u8 report_extended_ip_config_change;
}__attribute__((packed));

struct qmi_wds_indication_register_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_svc_sdu_header sdu_header;
	
	u16 message_id;
	u16 length;
	
	struct qmi_wds_extended_ip_configuration_change report_extended_ip_config_change;
}__attribute__((packed));

struct qmi_wds_changed_ip_config {
	u8 type;
	u16 length;
	u32 changed_ip_config;
}__attribute__((packed));

struct qmi_wds_runtime_requested {
	u8 type;
	u16 length;
	u32 requested_settings;
}__attribute__((packed));

struct qmi_wds_get_runtime_settings_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_svc_sdu_header sdu_header;
	
	u16 message_id;
	u16 length;
	
	struct qmi_wds_runtime_requested requested_settings;
}__attribute__((packed));

struct qmi_wds_ipv4_address_pref {
	u8 type;
	u16 length;
	u32 ipv4_address;
}__attribute__((packed));

struct qmi_wds_ipv6_address_pref {
	u8 type;
	u16 length;
	u8 ipv6_address[16];
	u8 ipv6_prefix_len;
}__attribute__((packed));

struct qmi_wds_ipv6_address_dns {
	u8 type;
	u16 length;
	u8 ipv6_address[16];
}__attribute__((packed));

struct qmi_wds_mtu {
	u8 type;
	u16 length;
	u32 mtu;
}__attribute__((packed));

struct qmi_wds_current_channel_rate {
	u8 type;
	u16 length;
	u32 current_tx_rate;
	u32 current_rx_rate;
	u32 max_tx_rate;
	u32 max_rx_rate;
}__attribute__((packed));

struct qmi_wds_get_current_channel_rate_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_svc_sdu_header sdu_header;
	
	u16 message_id;
	u16 length;
}__attribute__((packed));

struct qmi_wds_get_pkt_srvc_status_req {
	struct qmi_qmux_msg_header msg_header;
	struct qmi_svc_sdu_header sdu_header;
	
	u16 message_id;
	u16 length;
}__attribute__((packed));
