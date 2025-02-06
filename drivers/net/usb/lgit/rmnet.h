#define RMNET_RESPONSE_BUFFER_LEN	4096
#define RMNET_RESPONSE_TIMEOUT_MS	5000

#define RMNET_MAX_SERVICE_COUNT		16

#define RMNET_QMI_VERSION_CTL 0
#define RMNET_QMI_VERSION_WDS 1
#define RMNET_QMI_VERSION_DMS 2
#define RMNET_QMI_VERSION_NAS 3
#define RMNET_QMI_VERSION_MAX 4
#define RMNET_QMI_VERSION_WDA 0x1A

#define RMNET_INITIALIZE_LIMIT	50

#define RMNET_STATUS_VERSION		0x0001
#define RMNET_STATUS_SYNC			0x0002
#define RMNET_STATUS_REMOVED		0x0004
#define RMNET_STATUS_SET_INSTANCE	0x0008
#define RMNET_STATUS_QMI_INIT		0x0010

#define CONNECTION_CONNECTED	0x0
#define CONNECTION_DISCONNECTED	0x1

#define RMNET_RESTART_TIMER_DATAGRAM_CNT	2
#define RMNET_TIMER_PENDING_CNT				1
#define RMNET_TIMER_INTERVAL_USEC			100UL
#define RMNET_TIMER_INTERVAL_MIN			5UL
#define RMNET_TIMER_INTERVAL_MAX			(U32_MAX / NSEC_PER_USEC)

#define MAX_QMI_CTL_TXID_HISTORY 5


struct qmap_header {
	u8 padding;
	u8 mux_id;
	u16 data_len;
}__attribute__ ((packed));

struct qmap_control {
	u8 command_name;
	u8 command_type;
	u16 reserved;
	u32 transaction_id;
	u32 command_data[];
}__attribute__ ((packed));

struct qmi_version_info {
	u16 major_ver;
	u16 minor_ver;
}__attribute__ ((packed));

struct rmnet_context {
	struct usbnet *dev;
	struct net_device *netdev;
	struct usb_device *udev;
	struct usb_interface *intf;
	
	struct mutex cxt_lock;
	
	struct work_struct rmnet_carrier_on_work;
	struct work_struct rmnet_carrier_delayed_on_work;
	struct work_struct rmnet_initialize_work;
	struct work_struct rmnet_read_response_work;
	struct work_struct rmnet_txpath_work;

	struct hrtimer tx_timer;
	struct sk_buff *tx_skb;
	struct sk_buff *rem_skb;
	spinlock_t tx_lock;
	u32 timer_interval;
	u32 tx_timer_pending;
	atomic_t tx_flow_stop;
	
	u32 tx_datagrams;
	u32 tx_payloads;
	u32 tx_max_datagrams;
	u32 tx_max_size;
	u32 rx_max_datagrams;
	u32 rx_max_size;

	dev_t devno;
	struct cdev cdev;

	struct usb_host_endpoint *ep_read;
	struct usb_host_endpoint *ep_write;
	struct usb_host_endpoint *ep_interrupt;
	struct urb               *interrupt_urb;
	char                     *interrupt_buf;

	struct qmi_version_info qmi_version[RMNET_QMI_VERSION_MAX];

	struct list_entry svc_list_head;

	unsigned long rmnet_status;
	u8 qmi_ctl_txid;
	struct list_entry tx_list_head;
	
	int qmi_wda_client_id;
	int qmi_wda_txid;

	int qmi_wds_client_id;
	int qmi_wds_client_ex;
	int qmi_wds_txid;

	int qmi_dms_clinet_id;
	int qmi_nas_client_id;

	int connect_v4;
	u32 ipv4_address;
	int connect_v6;
	u8 ipv6_address[16];
	u8 ipv6_prefix_len;

	u8 real_dev_idx;
	u8 mux_dev_idx;
	u8 mux_id;
	bool is_make_char_dev;

}__attribute__ ((packed));


struct dev_instance_map{
	struct usb_interface* real_dev; //physical device
    struct usbnet* mux_dev_list[MAX_RMNET_MUX_DEVICE_NUM] ; //virtual devices for mux ports
    u8 mux_dev_num;
	u8 qmi_clt_txid;
	u8 remove_pending;
	atomic_t cdc_noti_read_count;
	struct work_struct qmi_init_work;
};

//int find_device_instance(void);
void rmnet_initialize_work(struct work_struct *work);
void rmnet_initialize(struct rmnet_context *context);
int rmnet_terminate(struct rmnet_context *context);
int rmnet_create_device(struct usbnet *dev);
void rmnet_destroy_device(struct usbnet *dev);
void rmnet_read_response_work(struct work_struct *work);
void rmnet_read_response(struct usbnet *dev);
void rmnet_find_endpoints(struct rmnet_context *context, struct usb_interface *intf);
void rmnet_free(struct rmnet_context *context);
void dumpspeed(struct usbnet *dev, __le32 *speeds);
int rmnet_status_start(struct rmnet_context *context);
void rmnet_status_stop(struct rmnet_context *context);
void rmnet_ctl_and_wda_initialize(struct rmnet_context *context);
void rmnet_initialize_work2(struct rmnet_context *context);
int find_rmnet_context(struct rmnet_context* context, void* buffer, struct rmnet_context** find_rmnet_context);
int find_rmnet_context_with_mux_id(u8 mux_id, struct rmnet_context** find_rmnet_context, u8 real_dev_idx);
int find_real_dev(struct usb_interface* real_dev);
int set_remove_pending(int real_dev_idx, u8 set_value);
int get_remove_pending(int real_dev_idx, u8* set_value);
void cleanup_real_dev(int real_dev_idx);
int add_real_dev(struct usb_interface* real_dev);
int add_mux_dev(int real_dev_idx, struct usbnet* mux_dev);
struct usbnet* get_mux_dev(int real_dev_idx, u8 mux_dev_idx);
struct usb_interface* get_real_dev(int real_dev_idx);
u8 get_mux_dev_num(int real_dev_idx);
u8 qmi_ctl_get_txid(struct rmnet_context *context);
int find_rmnet_context_with_svc_context(struct rmnet_context* context, void* buffer, struct rmnet_context** find_rmnet_context);