#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/workqueue.h>
#include <linux/mii.h>
#include <linux/cdev.h>
#include <linux/usb.h>
#include <linux/usb/cdc.h>
#include <linux/usb/usbnet.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#include <linux/ip.h>
#include <linux/ctype.h>
#include <linux/hrtimer.h>

#include "config.h"
#include "util.h"
#include "rmnet.h"
#include "qmi_ctl.h"
#include "qmi_svc.h"
#include "qmi_svc_wds.h"

static struct dev_instance_map g_dev_instance_map[MAX_CONNECT_DEVICES]; //device and adapter map;
static int real_dev_num; // current connected real(physical) device number;

static int rmnet_mux_device_num = 0;

struct class *rmnet_service_class;

/*--------------------------------------------------------------------*/

/*
 * TX timer callback
 *
 * Parameters
 *   timer : timer parameter
 *
 * Return value
 *   
 */
enum hrtimer_restart rmnet_tx_timer_cb(struct hrtimer *timer)
{
	struct rmnet_context *context = container_of(timer, struct rmnet_context, tx_timer);

	if (!(RMNET_STATUS_REMOVED & context->rmnet_status)) {
		schedule_work(&context->rmnet_txpath_work);
	}

	return HRTIMER_NORESTART;
}

/*
 * Get qmi control transaction id
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   qmi control transaction id
 */
u8 qmi_ctl_get_txid(struct rmnet_context *context)
{
	context->qmi_ctl_txid = g_dev_instance_map[context->real_dev_idx].qmi_clt_txid++;

	if(context->qmi_ctl_txid == QMI_CTL_MAX_TXID){
		g_dev_instance_map[context->real_dev_idx].qmi_clt_txid = QMI_CTL_MIN_TXID;
	}

	return context->qmi_ctl_txid;
}

/*
 * TX timer start
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   NONE
 */
void rmnet_tx_timeout_start(struct rmnet_context *context)
{
	if (!hrtimer_active(&context->tx_timer) && !(RMNET_STATUS_REMOVED & context->rmnet_status)) {
		hrtimer_start(&context->tx_timer, ktime_set(0, context->timer_interval), HRTIMER_MODE_REL);
	}
}

/*
 * TX start APC
 *
 * Parameters
 *   work : APC parameter
 *
 * Return value
 *   NONE
 */
void rmnet_txpath_work(struct work_struct *work)
{
	struct usbnet *dev;
	struct rmnet_context *context;

	context = container_of(work, struct rmnet_context, rmnet_txpath_work);
	dev = context->dev;
	
	spin_lock_bh(&context->tx_lock);
	if (context->tx_timer_pending != 0) {
		context->tx_timer_pending--;
		rmnet_tx_timeout_start(context);
		spin_unlock_bh(&context->tx_lock);
	} else if (dev->net != NULL) {
		spin_unlock_bh(&context->tx_lock);
		netif_tx_lock_bh(dev->net);
		usbnet_start_xmit(NULL, dev->net);
		netif_tx_unlock_bh(dev->net);
	} else {
		spin_unlock_bh(&context->tx_lock);
	}
}


/*
 * start qmi initialize work
 *
 * Parameters
 *   work : APC parameter
 *
 * Return value
 *   NONE
 */

void rmnet_initialize_work(struct work_struct *work){

	struct dev_instance_map *real_dev;
	struct rmnet_context *context;
	u8 i;
	u8 remove_pending = 0;

	real_dev = container_of(work, struct dev_instance_map, qmi_init_work);

	pr_debug(NET_DEVICE_NAME " start work item - rmnet_initialize_work.\n");

	if(real_dev == NULL){
		pr_err(NET_DEVICE_NAME " rmnet_initialize_work - get real_dev ptr fail. return\n");
		return;
	}

	for(i = 0; i < real_dev->mux_dev_num; i++){

		if(real_dev->mux_dev_list[i] == NULL){
			pr_err(NET_DEVICE_NAME " rmnet_initialize_work - mux dev (%d) is null ..\n", i);
			return;
		}

		context = (struct rmnet_context *)(real_dev->mux_dev_list[i]->data[0]);

		if(context == NULL){
			pr_err(NET_DEVICE_NAME " rmnet_initialize_work - interface context (%d) is null ..\n", i);
			return;
		}

		if(get_remove_pending(context->real_dev_idx, &remove_pending) < 0){
			pr_err(NET_DEVICE_NAME " rmnet_initialize_work - remove pending check (%d)\n", i);
			return;
		}

		if(remove_pending == 1){
			pr_err(NET_DEVICE_NAME " rmnet_initialize_work - remove pending condition (%d)\n", i);
			return;		
		}

		if (RMNET_STATUS_REMOVED & context->rmnet_status) {
			pr_err(NET_DEVICE_NAME " rmnet_initialize_work : device already removed (%d)\n", i);
			return;
		}

		rmnet_initialize(context);		

	}

	pr_debug(NET_DEVICE_NAME " end work item - rmnet_initialize_work.\n");
	
}

/*
 * carrier status change
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   NONE
 */
void rmnet_carrier_on(struct rmnet_context *context, u8 delay)
{
	// if device removed then end workitem
	if (RMNET_STATUS_REMOVED & context->rmnet_status) {
		pr_warn(NET_DEVICE_NAME " no carrier on - device removed\n");
		return;
	}
	
	if (delay > 0) schedule_timeout(delay * HZ);

	if (context->connect_v4 == CONNECTION_CONNECTED) {
		qmi_wds_get_runtime_settings(
			context,
			(u16)qmi_svc_get_txid(&context->qmi_wds_txid),
			context->qmi_wds_client_id);
	}

	if (context->connect_v6 == CONNECTION_CONNECTED) {
		qmi_wds_get_runtime_settings(
			context,
			(u16)qmi_svc_get_txid(&context->qmi_wds_txid),
			context->qmi_wds_client_ex);
	}

	qmi_wds_set_event_report(
		context,
		(u16)qmi_svc_get_txid(&context->qmi_wds_txid),
		context->qmi_wds_client_id);

	qmi_wds_get_current_channel_rate(
		context,
		(u16)qmi_svc_get_txid(&context->qmi_wds_txid),
		context->qmi_wds_client_id);
	
	// change carrier status
	if (context->connect_v4 == CONNECTION_CONNECTED ||
		context->connect_v6 == CONNECTION_CONNECTED) {
		pr_info(NET_DEVICE_NAME " link change to connected\n");
		netif_carrier_on(context->netdev);
	} else {
		pr_info(NET_DEVICE_NAME " no carrier on - already disconnected\n");
	}
}

/*
 * Carrier on APC
 *
 * Parameters
 *   work : APC parameter
 *
 * Return value
 *   NONE
 */
void rmnet_carrier_on_work(struct work_struct *work)
{
	struct rmnet_context *context;
	context = container_of(work, struct rmnet_context, rmnet_carrier_on_work);
	pr_debug(NET_DEVICE_NAME " carrier on work\n");
	rmnet_carrier_on(context, 0);
}

/*
 * Carrier delayed on APC
 *
 * Parameters
 *   work : APC parameter
 *
 * Return value
 *   NONE
 */
void rmnet_carrier_delayed_on_work(struct work_struct *work)
{
	struct rmnet_context *context;
	context = container_of(work, struct rmnet_context, rmnet_carrier_delayed_on_work);
	pr_debug(NET_DEVICE_NAME " carrier delayed on work\n");
	rmnet_carrier_on(context, 1);
}

/*
 * Get real device index
 *
 * Parameters
 *   real_dev : usb device information.
 *
 * Return value
 *   usb device index number.
 */
int find_real_dev(struct usb_interface* real_dev){
	
	int i;

	for(i = 0; i < MAX_CONNECT_DEVICES; i++){
		
		if(g_dev_instance_map[i].real_dev == real_dev){
			return i;
		}
	}
	
	return -1;
}

/*
 * Clean up usb deivce information in g_dev_instance_map
 *
 * Parameters
 *   real_dev_idx : usb device index number.
 *
 * Return value
 *   NONE
 */
void cleanup_real_dev(int real_dev_idx){
	int i;

	if(real_dev_idx < 0){
		return;
	}

	for(i = 0; i < rmnet_mux_device_num; i++){
		g_dev_instance_map[real_dev_idx].mux_dev_list[i] = NULL;
	}

	g_dev_instance_map[real_dev_idx].mux_dev_num = 0;
	g_dev_instance_map[real_dev_idx].qmi_clt_txid = QMI_CTL_MIN_TXID;
	g_dev_instance_map[real_dev_idx].real_dev = NULL;
	atomic_set(&g_dev_instance_map[real_dev_idx].cdc_noti_read_count, 0);
	
	real_dev_num--;

}

/*
 * Register usb deivce information in g_dev_instance_map
 *
 * Parameters
 *   real_dev : usb device information for register.
 *
 * Return value
 *   return index number of registered usb device
 */
int add_real_dev(struct usb_interface* real_dev){

	int i, j;
	
	for(i = 0; i < MAX_CONNECT_DEVICES; i++){
		
		if(g_dev_instance_map[i].real_dev == NULL){
			
			g_dev_instance_map[i].real_dev = real_dev;
			real_dev_num++;

			for(j = 0 ; j < rmnet_mux_device_num; j++){
				
				g_dev_instance_map[i].mux_dev_list[j] = NULL;
			}
			
			g_dev_instance_map[i].mux_dev_num = 0;
			g_dev_instance_map[i].qmi_clt_txid = QMI_CTL_MIN_TXID;
			g_dev_instance_map[i].remove_pending = 0;

			INIT_WORK(&g_dev_instance_map[i].qmi_init_work, rmnet_initialize_work);
			
			return i;
		}
	}

	return -1;
}

int set_remove_pending(int real_dev_idx, u8 set_value){

	if(real_dev_idx < 0){
		return -1;
	}

	if(g_dev_instance_map[real_dev_idx].real_dev == NULL){
		return -1;
	}

	g_dev_instance_map[real_dev_idx].remove_pending = set_value;
	return 0;
}


int get_remove_pending(int real_dev_idx, u8* set_value){

	if(real_dev_idx < 0){
		return -1;
	}

	if(g_dev_instance_map[real_dev_idx].real_dev  == NULL){
		return -1;
	}

	*set_value = g_dev_instance_map[real_dev_idx].remove_pending;
	return 0;
}

/*
 * Register rmnet ethernet adapter information in g_dev_instance_map
 *
 * Parameters
 *   real_dev_idx : usb device index
 *   mux_dev : rmnet ethernet adapter information for register
 *
 * Return value
 *   return index number of registered rmnet ethernet adapter
 */
int add_mux_dev(int real_dev_idx, struct usbnet* mux_dev){
	int i;

	if(real_dev_idx < 0){
		return -1;
	}

	for(i = 0; i < rmnet_mux_device_num; i++){
		if(g_dev_instance_map[real_dev_idx].mux_dev_list[i] == NULL){
			g_dev_instance_map[real_dev_idx].mux_dev_list[i] = mux_dev;
			g_dev_instance_map[real_dev_idx].mux_dev_num++;
			return i;
		}
	}

	return -1;
}

/*
 * Clean up rmnet ethernet adapter information in g_dev_instance_map
 *
 * Parameters
 *   real_dev_idx : usb device index
 *   mux_dev : rmnet ethernet adapter information for clean up
 *
 * Return value
 *   return index number of removed rmnet ethernet adapter
 */
int cleanup_mux_dev(int real_dev_idx, struct usbnet* mux_dev){
	int i;

	if(real_dev_idx < 0){
		return -1;
	}

	for(i = 0; i < rmnet_mux_device_num; i++){
		if(g_dev_instance_map[real_dev_idx].mux_dev_list[i] == mux_dev){
			g_dev_instance_map[real_dev_idx].mux_dev_list[i] = NULL;
			g_dev_instance_map[real_dev_idx].mux_dev_num--;
			return i;
		}
	}

	return -1;
}

/*
 * Get rmnet ethernet adapter information
 *
 * Parameters
 *   real_dev_idx : usb device index
 *   mux_dev_idx : rmnet ethernet adapter index
 *
 * Return value
 *   return rmnet ethernet adapter information
 */
struct usbnet* get_mux_dev(int real_dev_idx, u8 mux_dev_idx){
	return g_dev_instance_map[real_dev_idx].mux_dev_list[mux_dev_idx];
}

/*
 * Get usb device information
 *
 * Parameters
 *   real_dev_idx : usb device index
 *
 * Return value
 *   return usb device information
 */
struct usb_interface* get_real_dev(int real_dev_idx){
	return g_dev_instance_map[real_dev_idx].real_dev;
}


/*
 * Get number of mux devices.
 *
 * Parameters
 *   real_dev_idx : usb device index
 *
 * Return value
 *   return current number of logical mux devices.
 */
u8 get_mux_dev_num(int real_dev_idx){
	return g_dev_instance_map[real_dev_idx].mux_dev_num;
}

/*
 * use usb data packet for find real rmnet context.
 *
 * Parameters
 *   context : unidentified rmnet context.
 *   buffer : packet data sent by usb device.
 *   find_rmnet_context : <out param> real rmnet context ptr.
 *
 * Return value
 *   '-1' : funtion is fail.
 *   '0' : funtion is success.
 */
int find_rmnet_context(struct rmnet_context* context, void* buffer, struct rmnet_context** find_rmnet_context){
	
	int status = -1;
	int i = 0;
	u8 find_svc_id = 0;
	u8 find_svc_id2 = 0; //for qmi wds ipv4 and ipv6
	u8 find_txid = 0;
	struct qmi_svc_response_header *svc_header = (struct qmi_svc_response_header *)buffer;
	struct qmi_ctl_response_header *ctl_header = (struct qmi_ctl_response_header *)buffer;
	struct rmnet_context* cur_rmnet_context = NULL;

	*find_rmnet_context = NULL;

	if(g_dev_instance_map[context->real_dev_idx].real_dev == NULL){
		return status;
	}
		
	for(i = 0; i < rmnet_mux_device_num; i++){

		if(g_dev_instance_map[context->real_dev_idx].mux_dev_list[i] == NULL){
			continue;
		}

		cur_rmnet_context = (struct rmnet_context*) g_dev_instance_map[context->real_dev_idx].mux_dev_list[i]->data[0];

		if(cur_rmnet_context == NULL){
			continue;
		}

		switch(svc_header->msg_header.service_type){
		case RMNET_QMI_VERSION_WDA :
			find_svc_id = cur_rmnet_context->qmi_wda_client_id;
			break;
		case RMNET_QMI_VERSION_CTL :
			find_txid = cur_rmnet_context->qmi_ctl_txid;
			break;		
		case RMNET_QMI_VERSION_WDS :
			find_svc_id = cur_rmnet_context->qmi_wds_client_ex;
			find_svc_id2 = cur_rmnet_context->qmi_wds_client_id;
			break;
		case RMNET_QMI_VERSION_DMS :
			find_svc_id = cur_rmnet_context->qmi_dms_clinet_id;
			break;
		case RMNET_QMI_VERSION_NAS :
			find_svc_id = cur_rmnet_context->qmi_nas_client_id;
			break;
		}

		if(svc_header->msg_header.service_type == RMNET_QMI_VERSION_CTL){

			if(ctl_header->sdu_header.transaction_id == find_txid){
				*find_rmnet_context = cur_rmnet_context;
				status = 0;
				break;
			}
			
		}else if(svc_header->msg_header.service_type == RMNET_QMI_VERSION_WDS){

			if(find_svc_id == svc_header->msg_header.client_id){
				*find_rmnet_context = cur_rmnet_context;
				status = 0;
				break;
			}else if(find_svc_id2 == svc_header->msg_header.client_id){
				*find_rmnet_context = cur_rmnet_context;
				status = 0;
				break;			
			}
		}else{
		
			if(find_svc_id == svc_header->msg_header.client_id){
				
				*find_rmnet_context = cur_rmnet_context;
				status = 0;
				break;
			}
		}
		
	}

	return status;
	
}

/*
 * use mux id for find real rmnet context
 *
 * Parameters
 *   mux_id : rmnet mux id.
 *   find_rmnet_context : <out param> real rmnet context ptr.
 *   real_dev_idx : usb device index.
 *
 * Return value
 *   '-1' : funtion is fail.
 *   '0' : funtion is success.
 */
int find_rmnet_context_with_mux_id(u8 mux_id, struct rmnet_context** find_rmnet_context, u8 real_dev_idx){
	
	int status = -1;
	int i = 0;
	struct rmnet_context* cur_rmnet_context = NULL;

	*find_rmnet_context = NULL;

	if(g_dev_instance_map[real_dev_idx].real_dev == NULL){
		return status;
	}


	for(i = 0; i < rmnet_mux_device_num; i++){

		if(g_dev_instance_map[real_dev_idx].mux_dev_list[i] == NULL){
			continue;
		}

		cur_rmnet_context = (struct rmnet_context*)g_dev_instance_map[real_dev_idx].mux_dev_list[i]->data[0];

		if(cur_rmnet_context == NULL){
			continue;
		}
		
		if(mux_id == cur_rmnet_context->mux_id){
			
			*find_rmnet_context = cur_rmnet_context;
			status = 0;
			break;
		}	
	}
	
	return status;
}

/*
 * use qmi packet for find real rmnet context
 *
 * Parameters
 *   context : unidentified rmnet context
 *   buffer : qmi packet
 *   find_rmnet_context : <out param> real rmnet context ptr
 *
 * Return value
 *   '-1' : funtion is fail.
 *   '0' : funtion is success.
 */
int find_rmnet_context_with_svc_context(struct rmnet_context* context, void* buffer, struct rmnet_context** find_rmnet_context){
	
	int real_dev_idx = context->real_dev_idx;
	int i;
	
	struct qmi_svc_response_header *svc_header = (struct qmi_svc_response_header *)buffer;
	
	struct rmnet_context* cur_rmnet_context = NULL;
	struct qmi_svc_context* cur_qmi_svc_context = NULL;
	struct list_entry* node = NULL;
	
	*find_rmnet_context = NULL;


	if(g_dev_instance_map[real_dev_idx].real_dev == NULL){
		return -1;
	}

	for(i = 0; i < rmnet_mux_device_num; i ++){

		if(g_dev_instance_map[real_dev_idx].mux_dev_list[i] == NULL){
			continue;
		}

		cur_rmnet_context =  (struct rmnet_context*)g_dev_instance_map[real_dev_idx].mux_dev_list[i]->data[0];

		if(cur_rmnet_context == NULL){
			continue;
		}
		
		mutex_lock(&cur_rmnet_context->cxt_lock);
		node = &cur_rmnet_context->svc_list_head;

		if(node == NULL){
			mutex_unlock(&cur_rmnet_context->cxt_lock);
			continue;
		}
		
		while (node->next != &cur_rmnet_context->svc_list_head) {
			node = node->next;
			cur_qmi_svc_context = (struct qmi_svc_context *)container_of(node, struct qmi_svc_context, list);
			
			if (cur_qmi_svc_context == NULL) {
				continue;
			}
			
			if(cur_qmi_svc_context->service_type == svc_header->msg_header.service_type &&
				cur_qmi_svc_context->client_id == svc_header->msg_header.client_id){

				*find_rmnet_context = cur_rmnet_context;
				mutex_unlock(&cur_rmnet_context->cxt_lock);
				
				return 0;
			}
		}

		mutex_unlock(&cur_rmnet_context->cxt_lock);

	}

	return -1;
	
}

/*
 * initialize qmi wda for rmnet device.
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   NONE
 */
void rmnet_ctl_and_wda_initialize(struct rmnet_context *context){

	int i = 0;
	int status = 0;
	u8 client_id = 0;

	// get qmi version info
	for ( ; i < RMNET_INITIALIZE_LIMIT ; i++) {
		if (RMNET_STATUS_REMOVED & context->rmnet_status) goto exit;
		pr_info(NET_DEVICE_NAME " QMI version checking\n");
		status = qmi_ctl_get_version_info(context);
		if (status < 0) {
			pr_warn(NET_DEVICE_NAME " Get QMI version info error %d\n", status);
			goto exit;
		} else if (status > 0) {
			pr_info(NET_DEVICE_NAME " Get QMI version info success\n");
			context->rmnet_status |= RMNET_STATUS_VERSION;
			break;
		}
	}

	// set qmi sync
	for ( ; i < RMNET_INITIALIZE_LIMIT ; i++) {
		if (RMNET_STATUS_REMOVED & context->rmnet_status) goto exit;
		status = qmi_ctl_sync(context);
		if (status < 0) {
			pr_warn(NET_DEVICE_NAME " QMI sync error %d\n", status);
			goto exit;
		} else if (status > 0) {
			pr_info(NET_DEVICE_NAME " QMI sync success\n");
			context->rmnet_status |= RMNET_STATUS_SYNC;
			break;
		}
	}
	
	for ( ; i < RMNET_INITIALIZE_LIMIT ; i++) {
		if (RMNET_STATUS_REMOVED & context->rmnet_status) goto exit;
		status = qmi_ctl_set_instance_id(context);
		if (status < 0) {
			pr_warn(NET_DEVICE_NAME " QMI set instance id error %d\n", status);
			goto exit;
		} else if (status > 0) {
			pr_info(NET_DEVICE_NAME " QMI set instance id success\n");
			context->rmnet_status |= RMNET_STATUS_SET_INSTANCE;
			break;
		}
	}

	pr_debug(NET_DEVICE_NAME " Control initialize success - spend %d times\n", i);
		
	// open WDA service for set data format
	client_id = 0;
	status = qmi_ctl_get_client_id(context, RMNET_QMI_VERSION_WDA, &client_id);
	
	if (status < 0) {
		printk(KERN_ERR "WDA service open error [%d %d] \n", context->real_dev_idx, context->mux_dev_idx);
		goto exit;
	}
	
	context->qmi_wda_client_id = (int)client_id;
	context->qmi_wda_txid = QMI_SVC_MIN_TXID;
	
	pr_debug(NET_DEVICE_NAME " WDA service opened - client id = 0x%x\n", context->qmi_wda_client_id);
	
	status = qmi_wda_set_data_format(context);
	if (status < 0) {
		printk(KERN_ERR "WDA set data format error [%d %d] %d\n", context->real_dev_idx, context->mux_dev_idx, status);
	} else {
		printk(KERN_NOTICE "WDA set data format success [%d %d] \n", context->real_dev_idx, context->mux_dev_idx);
	}
	
	status = qmi_wda_set_qmap_settings(context);
	if (status < 0) {
		printk(KERN_ERR "qmi_wda_set_qmap_settings error [%d %d] %d \n", context->real_dev_idx, context->mux_dev_idx, status);
	} else {
		printk(KERN_NOTICE "qmi_wda_set_qmap_settings success [%d %d] \n", context->real_dev_idx, context->mux_dev_idx);
	}

	qmi_ctl_release_client_id(context, RMNET_QMI_VERSION_WDA, context->qmi_wda_client_id);
	
	if (status < 0) {
		printk(KERN_NOTICE "WDA service close error [%d %d] %d\n", context->real_dev_idx, context->mux_dev_idx, status);
	}else {
		printk(KERN_NOTICE "WDA service close success [%d %d] \n", context->real_dev_idx, context->mux_dev_idx);
	}
	
	context->qmi_wda_client_id = -1;
	context->qmi_wda_txid = QMI_SVC_MIN_TXID;

exit :

	return;
	
}

/*
 * initialize qmi wds for rmnet ethernet adapter.
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   NONE
 */
void rmnet_initialize_work2(struct rmnet_context *context)
{
	int status = 0;

	int i;
	u8 client_id;
	
	pr_info(NET_DEVICE_NAME " rmnet initialize worker\n");

	printk(KERN_NOTICE "==rmnet_initialize_work start [%d %d]\n", context->real_dev_idx, context->mux_dev_idx);


	client_id = 0;

	context->qmi_wds_client_id= -1;
	context->qmi_wds_txid = QMI_SVC_MIN_TXID;

	//start ipv4 wds service

	for(i = 0; i < 5; i++){
		
		status = qmi_ctl_get_client_id(context, RMNET_QMI_VERSION_WDS, &client_id);
		
		if(status < 0) {
			printk(KERN_ERR "ipv4 WDS service open error [%d] [client_id :%d] \n", context->real_dev_idx, client_id);
		}else if(client_id == 0){
			printk(KERN_ERR "ipv4 invalid WDS service client id [%d] [client_id :%d] \n", context->real_dev_idx, client_id);
		}else{
			printk(KERN_NOTICE "ipv4 WDS service open success [%d] [client_id :%d] \n", context->real_dev_idx, client_id);
			break;
		}
	}

	if(status < 0 || client_id == 0){
		printk(KERN_ERR "finally... ipv4 invalid WDS service client id [%d] [client_id :%d] \n", context->real_dev_idx, client_id);
		goto exit;		
	}

	context->qmi_wds_client_id = (int)client_id;

	status = qmi_wds_bind_mux_data_port(
			context,
			(u16)qmi_svc_get_txid(&context->qmi_wds_txid),
			context->qmi_wds_client_id,
			context->mux_id);
	if (status <= 0) {
		printk(KERN_ERR "ipv4 qmi_wds_bind_mux_data_port fail. [status %d] [%d]\n", status, context->real_dev_idx);
		goto exit;
	}else{
		printk(KERN_NOTICE "ipv4 qmi_wds_bind_mux_data_port success. [status %d] [%d]\n", status, context->real_dev_idx);
	}

	status = qmi_wds_set_client_ip_family(
		context,
		(u16)qmi_svc_get_txid(&context->qmi_wds_txid),
		context->qmi_wds_client_id,
		QMI_WDS_IP_FAMILY_IPV4);
	
	if (status < 0) {
		pr_warn(NET_DEVICE_NAME " WDS ipv4 set client ip family pref error %d\n", status);
	}
	
	status = qmi_wds_indication_register(
				context,
				(u16)qmi_svc_get_txid(&context->qmi_wds_txid),
				context->qmi_wds_client_id);
	if (status < 0) {
		pr_warn(NET_DEVICE_NAME " WDS ipv4 indication register error %d\n", status);
	}

	//start ipv6 wds service

	client_id = 0;

	context->qmi_wds_client_ex = -1;
	context->qmi_wds_txid = QMI_SVC_MIN_TXID;

	for(i = 0; i < 5; i++){
		
		status = qmi_ctl_get_client_id(context, RMNET_QMI_VERSION_WDS, &client_id);
		
		if(status < 0) {
			printk(KERN_ERR "ipv6 WDS service open error [%d] [client_id :%d] \n", context->real_dev_idx, client_id);
		}else if(client_id == 0){
			printk(KERN_ERR "ipv6 invalid WDS service client id [%d] [client_id :%d] \n", context->real_dev_idx, client_id);
		}else{
			printk(KERN_NOTICE "ipv6 WDS service open success [%d] [client_id :%d] \n", context->real_dev_idx, client_id);
			break;
		}
	}

	if(status < 0 || client_id == 0){
		printk(KERN_ERR "finally... ipv6 invalid WDS service client id [%d] [client_id :%d] \n", context->real_dev_idx, client_id);
		goto exit;		
	}

	context->qmi_wds_client_ex = (int)client_id;

	status = qmi_wds_bind_mux_data_port(
			context,
			(u16)qmi_svc_get_txid(&context->qmi_wds_txid),
			context->qmi_wds_client_ex,
			context->mux_id);
	if (status <= 0) {
		printk(KERN_ERR "ipv6 qmi_wds_bind_mux_data_port fail. [status %d] [%d]\n", status, context->real_dev_idx);
		goto exit;
	}else{
		printk(KERN_NOTICE "ipv6 qmi_wds_bind_mux_data_port success. [status %d] [%d]\n", status, context->real_dev_idx);
	}
	
	status = qmi_wds_set_client_ip_family(
		context,
		(u16)qmi_svc_get_txid(&context->qmi_wds_txid),
		context->qmi_wds_client_ex,
		QMI_WDS_IP_FAMILY_IPV6);
	
	if (status < 0) {
		pr_warn(NET_DEVICE_NAME " WDS ipv6 set client ip family pref error %d\n", status);
	}
	

	status = qmi_wds_indication_register(
				context,
				(u16)qmi_svc_get_txid(&context->qmi_wds_txid),
				context->qmi_wds_client_ex);
	if (status < 0) {
		pr_warn(NET_DEVICE_NAME " WDS ipv6 indication register error %d\n", status);
	}

	context->rmnet_status |= RMNET_STATUS_QMI_INIT;

	printk(KERN_NOTICE "\n\n\n******************End rmnet init [real_dev : %d mux_dev : %d, ipv4_cid : %d, ipv6_cid : %d]******************\n", context->real_dev_idx, context->mux_dev_idx, context->qmi_wds_client_id, context->qmi_wds_client_ex);

exit :

	return;
}

/*
 * Rmnet initialize handler
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   none
 */
void rmnet_initialize(struct rmnet_context *context)
{
	// schedule initializer
	if(context->mux_dev_idx == 0){
		rmnet_ctl_and_wda_initialize(context);
	}

	rmnet_initialize_work2(context);
}


/*
 * Rmnet terminate handler
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   0
 */
int rmnet_terminate(struct rmnet_context *context)
{
	int status = 0;

	struct list_entry *entry;
	struct qmi_ctl_transaction_item *tx_item = NULL;

	pr_info(NET_DEVICE_NAME " rmnet terminate\n");

	// wakeup read thread
	mutex_lock(&context->cxt_lock);
	entry = &context->tx_list_head;
	while (entry->next != &context->tx_list_head) {
		entry = entry->next;
		tx_item = (struct qmi_ctl_transaction_item *)container_of(entry, struct qmi_ctl_transaction_item, list);
		pr_debug(NET_DEVICE_NAME " wakeup txitem (id:%d)\n", tx_item->transaction_id);
		tx_item->length = -1;
        mutex_unlock(&context->cxt_lock);
		if (tx_item->wait_queue_head) wake_up_interruptible(tx_item->wait_queue_head);
        mutex_lock(&context->cxt_lock);
	}
    mutex_unlock(&context->cxt_lock);

	// internal wds service release
	/*
	if (context->qmi_wds_client_id > 0) {
		status = qmi_ctl_release_client_id(context, RMNET_QMI_VERSION_WDS, context->qmi_wds_client_id);
		
		if(status < 0){
			printk(KERN_WARNING "rmnet_terminate - qmi_ctl_release_client_id fail 1 [%d %d] (%d)\n", context->real_dev_idx, context->mux_dev_idx, status);
		}
	}
	
	if (context->qmi_wds_client_ex > 0) {
		status = qmi_ctl_release_client_id(context, RMNET_QMI_VERSION_WDS, context->qmi_wds_client_ex);
		
		if(status < 0){
			printk(KERN_WARNING "rmnet_terminate - qmi_ctl_release_client_id fail 2 [%d %d] (%d)\n", context->real_dev_idx, context->mux_dev_idx, status);
		}
	}
	*/
	return status;
}

/*
 * Send CDC_SET_CONTROL_LINE_STATE
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   0
 */
int rmnet_set_line_state(struct rmnet_context *context)
{
	int status = 0;
	
	status = usb_control_msg(context->udev,
							 usb_sndctrlpipe(context->udev, 0),
							 USB_CDC_REQ_SET_CONTROL_LINE_STATE,
							 USB_TYPE_CLASS | USB_RECIP_INTERFACE,
							 0,
							 context->intf->cur_altsetting->desc.bInterfaceNumber,
							 NULL,
							 0,
							 RMNET_RESPONSE_TIMEOUT_MS);

	pr_info(NET_DEVICE_NAME " set control line state 0 status %d\n", status);

	status = usb_control_msg(context->udev,
							 usb_sndctrlpipe(context->udev, 0),
							 USB_CDC_REQ_SET_CONTROL_LINE_STATE,
							 USB_TYPE_CLASS | USB_RECIP_INTERFACE,
							 1,
							 context->intf->cur_altsetting->desc.bInterfaceNumber,
							 NULL,
							 0,
							 RMNET_RESPONSE_TIMEOUT_MS);

	pr_info(NET_DEVICE_NAME " set control line state 1 status %d\n", status);

	return status;
}

static const struct file_operations rmnet_service_fops = {
	.owner 			= THIS_MODULE,
	.read 			= qmi_svc_read,
	.write 			= qmi_svc_write,
	.unlocked_ioctl	= qmi_svc_ioctl,
	.open 			= qmi_svc_open,
	.flush 			= qmi_svc_flush,
	.release		= qmi_svc_release,
};

/*
 * Create rmnet character device
 *
 * Parameters
 *   dev : net device
 *
 * Return value
 *   < 0 : error
 *   = 0 : success
 */
int rmnet_create_device(struct usbnet *dev)
{
	int status = 0;
	struct rmnet_context *context;

	dev_t devno;

	context = (struct rmnet_context *)dev->data[0];
	
	status = alloc_chrdev_region(&devno, 0, 1, CHAR_DEVICE_NAME);

	if (status < 0) {
		pr_err(NET_DEVICE_NAME " alloc chrdev region failed 0x%08x\n", status);
		goto exit;
	}

	cdev_init(&context->cdev, &rmnet_service_fops);
	context->cdev.owner = THIS_MODULE;
	context->cdev.ops = &rmnet_service_fops;

	status = cdev_add(&context->cdev, devno, 1);

	if (status < 0) {
		pr_err(NET_DEVICE_NAME " cdev add failed 0x%08x\n", status);
		goto exit;
	}
	
	pr_info(NET_DEVICE_NAME " create character device prefix %s postfix\n", CHAR_DEVICE_NAME);

	device_create(rmnet_service_class, NULL, devno, NULL, "%s%d", CHAR_DEVICE_NAME, context->real_dev_idx);

	context->devno = devno;

exit : 

	return status;
}

/*
 * Destroy rmnet character device
 *
 * Parameters
 *   dev : net device
 *
 * Return value
 *   none
 */
void rmnet_destroy_device(struct usbnet *dev)
{
	struct rmnet_context *context;
	struct list_entry *entry;
	struct qmi_svc_context *svc_context;
	void *wait_queue = NULL;

	context = (struct rmnet_context *)dev->data[0];
	
	pr_info(NET_DEVICE_NAME " destroy character devices\n");

	mutex_lock(&context->cxt_lock);
	
	// wakeup wait notify
	while (context->svc_list_head.next != &context->svc_list_head) {
		entry = context->svc_list_head.next;
		svc_context = (struct qmi_svc_context *)container_of(entry, struct qmi_svc_context, list);
		if (svc_context == NULL) {
			continue;
		}

        remove_list(&svc_context->list);

		if (svc_context->read_wait_active > 0 &&
			svc_context->read_wait_queue != NULL) {
			svc_context->error_no = -ENXIO;
			pr_debug(NET_DEVICE_NAME " wake up device(context:0x%p) for read waiting by destroy\n", (void *)svc_context);
			svc_context->read_wait_active = 0;
			wait_queue = svc_context->read_wait_queue;
			svc_context->read_wait_queue = NULL;
			mutex_unlock(&context->cxt_lock);
			wake_up_interruptible(wait_queue);
			mutex_lock(&context->cxt_lock);
		}

		if(atomic_dec_return(&svc_context->ref_cnt) == 0){
			pr_debug(NET_DEVICE_NAME " rmnet_destroy_device - free memory - svc_context\n");
			kfree(svc_context);
		}
	}

	cleanup_mux_dev(context->real_dev_idx, dev);

	mutex_unlock(&context->cxt_lock);

	if(context->is_make_char_dev == true){

		schedule_timeout_interruptible(1 * HZ);
		
		device_destroy(rmnet_service_class, context->devno);
		cdev_del(&context->cdev);
		unregister_chrdev_region(context->devno, 1);
	}
}

/*
 * Read qmi response APC
 *
 * Parameters
 *   work : APC parameter
 *
 * Return value
 *   NONE
 */
void rmnet_read_response_work(struct work_struct *work)
{
	int status = 0;
	struct rmnet_context *context;

	char *buffer = NULL;
	struct qmi_qmux_msg_header *qmux_header = NULL;

	context = container_of(work, struct rmnet_context, rmnet_read_response_work);
	
	if (RMNET_STATUS_REMOVED & context->rmnet_status) {
		atomic_dec(&g_dev_instance_map[context->real_dev_idx].cdc_noti_read_count);
		return;
	}

	pr_debug(NET_DEVICE_NAME " read response data worker\n");

	// allocate read buffer
	buffer = kmalloc(RMNET_RESPONSE_BUFFER_LEN, GFP_KERNEL);
	if (!buffer) {
		pr_warn(NET_DEVICE_NAME " alloc read buffer error\n");
		atomic_dec(&g_dev_instance_map[context->real_dev_idx].cdc_noti_read_count);
		return;
	}
	
	memset(buffer, 0, RMNET_RESPONSE_BUFFER_LEN);

	while(atomic_read(&g_dev_instance_map[context->real_dev_idx].cdc_noti_read_count) > 0){

		atomic_dec(&g_dev_instance_map[context->real_dev_idx].cdc_noti_read_count);


		// usb read message
		status = usb_control_msg(context->udev,
								 usb_rcvctrlpipe(context->udev, 0),
								 USB_CDC_GET_ENCAPSULATED_RESPONSE,
								 USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
								 0,
								 context->intf->cur_altsetting->desc.bInterfaceNumber,
								 buffer,
								 RMNET_RESPONSE_BUFFER_LEN,
								 RMNET_RESPONSE_TIMEOUT_MS);

		if (status < 0) {
			pr_err(NET_DEVICE_NAME " usb read response error %d\n", status);
			printk(KERN_WARNING "usb read response error %d \n", status);
			

			if(atomic_read(&g_dev_instance_map[context->real_dev_idx].cdc_noti_read_count) > 0){
				continue;
			}else{
				goto out;
			}
			
		}

		qmux_header = (struct qmi_qmux_msg_header *)buffer;
		
		pr_debug(NET_DEVICE_NAME " read response %d bytes\n", status);
		DBG_DUMP_MEMORY(buffer, status);

		// dispatch response message
		if (qmux_header->service_type == 0) {
			qmi_ctl_proc_response(context, buffer, status);
		} else {
			qmi_svc_proc_response(context, buffer, status);
		}
		
	}

out :
	// free read buffer
	kfree(buffer);
}

/*
 * Read qmi response handler
 *
 * Parameters
 *   dev : net device
 *
 * Return value
 *   none
 */
void rmnet_read_response(struct usbnet *dev)
{
	struct rmnet_context *context;
	context = (struct rmnet_context *)dev->data[0];

	atomic_inc(&g_dev_instance_map[context->real_dev_idx].cdc_noti_read_count);

	// schedule usb read message
	if(schedule_work(&context->rmnet_read_response_work) == 0){
		printk(KERN_WARNING "schedule_work fail\n");
	}
}

/*-------------------------------------------------------------------------*/

/*
 * Find and save usb endpoints
 *
 * Parameters
 *   context : rmnet context
 *   intf : interface descriptor
 *
 * Return value
 *   NONE
 */
void rmnet_find_endpoints(struct rmnet_context *context, struct usb_interface *intf)
{
	struct usb_host_endpoint *e;
	u8 ep;

	// retreive endpoints
	for (ep = 0; ep < intf->cur_altsetting->desc.bNumEndpoints; ep++) {

		e = intf->cur_altsetting->endpoint + ep;
		switch (e->desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) {
		case USB_ENDPOINT_XFER_INT:
			if (usb_endpoint_dir_in(&e->desc)) {
				if (context->ep_interrupt == NULL)
					context->ep_interrupt = e;
			}
			break;

		case USB_ENDPOINT_XFER_BULK:
			if (usb_endpoint_dir_in(&e->desc)) {
				if (context->ep_read == NULL)
					context->ep_read = e;
			} else {
				if (context->ep_write == NULL)
					context->ep_write = e;
			}
			break;

		default:
			break;
		}
	}
}

/*
 * Free rmnet context
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   NONE
 */
void rmnet_free(struct rmnet_context *context)
{
	if (context == NULL)
		return;

	if (context->rem_skb != NULL) {
		dev_kfree_skb_any(context->rem_skb);
		context->rem_skb = NULL;
	}

	if (context->tx_skb != NULL) {
		dev_kfree_skb_any(context->tx_skb);
		context->tx_skb = NULL;
	}

	kfree(context);
}

/*
 * dump speed handler
 *
 * Parameters
 *   dev : usb device object
 *   speeds : link speed
 *
 * Return value
 *   NONE
 */
void dumpspeed(struct usbnet *dev, __le32 *speeds)
{
	netif_info(dev, timer, dev->net,
		   "link speeds: %u kbps up, %u kbps down\n",
		   __le32_to_cpu(speeds[0]) / 1000,
		   __le32_to_cpu(speeds[1]) / 1000);
}

/*-------------------------------------------------------------------------*/

/*
 * NIC i/o control handler
 *
 * Parameters
 *   dev : net device object
 *   ifr :
 *   cmd : i/o control command
 *
 * Return value
 *   NONE
 */
int rmnet_do_ioctl(struct net_device *net, struct ifreq *ifr, int cmd)
{
	struct usbnet *dev = netdev_priv(net);

	pr_debug(NET_DEVICE_NAME " do ioctl - dev : 0x%p cmd : %d\n", (void *)dev, cmd);

	// path through
	return generic_mii_ioctl(&dev->mii, if_mii(ifr), cmd, NULL);
}

/*
 * return driver informtion
 *
 * Parameters
 *   dev : net device object
 *   info : driver informations
 *
 * Return value
 *   NONE
 */
void rmnet_get_drvinfo(struct net_device *net, struct ethtool_drvinfo *info)
{
	struct usbnet *dev = netdev_priv(net);

	strncpy(info->driver, dev->driver_name, sizeof(info->driver));
	strncpy(info->version, DRIVER_VERSION, sizeof(info->version));
	strncpy(info->fw_version, dev->driver_info->description, sizeof(info->fw_version));
	usb_make_path(dev->udev, info->bus_info, sizeof(info->bus_info));
}

static const struct net_device_ops rmnet_netdev_ops = {
	.ndo_open				= usbnet_open,
	.ndo_stop				= usbnet_stop,
	.ndo_start_xmit			= usbnet_start_xmit,
	.ndo_tx_timeout			= usbnet_tx_timeout,
	.ndo_change_mtu			= usbnet_change_mtu,
	.ndo_set_mac_address	= eth_mac_addr,
	.ndo_validate_addr		= eth_validate_addr,
	.ndo_do_ioctl			= rmnet_do_ioctl,
};

static struct ethtool_ops rmnet_ethtool_ops = {
	.get_drvinfo	= rmnet_get_drvinfo,
	.get_link		= usbnet_get_link,
	.get_msglevel 	= usbnet_get_msglevel,
	.set_msglevel 	= usbnet_set_msglevel,
	.get_settings 	= usbnet_get_settings,
	.set_settings 	= usbnet_set_settings,
	.nway_reset 	= usbnet_nway_reset,
};

/*-------------------------------------------------------------------------*/

/*
 * bind handler
 *
 * Parameters
 *   dev : net device object
 *   intf : interface descriptor
 *
 * Return value
 *   < 0 : error
 *   = 0 : success
 */
int rmnet_bind(struct usbnet *dev, struct usb_interface *intf)
{
	int status = -1;
	struct rmnet_context *context;
	int real_dev_idx;
	int mux_dev_idx;
	
	// allocation rmnet context buffer
	context = kmalloc(sizeof(*context), GFP_KERNEL);
	if (context == NULL) {
		pr_err(NET_DEVICE_NAME " context allocation failed\n");
		return -ENODEV;
	}
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)
	dev_hw_addr_random(dev->net, dev->net->dev_addr);
#else
	eth_hw_addr_random(dev->net);
#endif
	if ((dev->net->dev_addr[0] & 0xd0) == 0x40) {
		dev->net->dev_addr[0] |= 0x02;
		dev->net->dev_addr[0] &= 0xbf;
	}
	
	real_dev_idx = find_real_dev(intf);

	if(real_dev_idx < 0){
		pr_err(NET_DEVICE_NAME " real_dev_idx error %d\n", status);
		goto error;
	}

	mux_dev_idx = add_mux_dev(real_dev_idx, dev); // this func will return mux dev instance number. if func is fail, stataus value is -1.

	if(mux_dev_idx < 0){
		pr_err(NET_DEVICE_NAME " add_mux_dev error %d\n", status);
		goto error;
	}
	
	dev->net->netdev_ops = &rmnet_netdev_ops;

	memset(context, 0, sizeof(*context));

	context->dev = dev;
	context->netdev = dev->net;
	context->udev = dev->udev;
	context->intf = intf;

	context->real_dev_idx = real_dev_idx;
	context->mux_dev_idx = mux_dev_idx;
	context->mux_id = QMAP_MUX_ID + mux_dev_idx;
	context->is_make_char_dev = false;

	
	/* store ctx pointer in device data field */
	dev->data[0] = (unsigned long)context;

	// initialize service context list and transaction list
	initialize_list_head(&context->svc_list_head);
	initialize_list_head(&context->tx_list_head);

	// find endpoints
	//status = usbnet_get_endpoints(dev, intf);
	rmnet_find_endpoints(context, intf);
	
	//if (status < 0) {
	if (!context->ep_read || !context->ep_write || !context->ep_interrupt) {
		pr_err(NET_DEVICE_NAME " get endpoint error\n");
		goto error;
	}

	dev->in = usb_rcvbulkpipe (dev->udev,
			context->ep_read->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
	dev->out = usb_sndbulkpipe (dev->udev,
			context->ep_write->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);

	dev->net->flags |= IFF_NOARP;
	dev->net->ethtool_ops = &rmnet_ethtool_ops;

	// not connected
	netif_carrier_off(context->netdev);

	INIT_WORK(&context->rmnet_carrier_on_work, rmnet_carrier_on_work);
	INIT_WORK(&context->rmnet_carrier_delayed_on_work, rmnet_carrier_delayed_on_work);
	//INIT_WORK(&context->rmnet_initialize_work, rmnet_initialize_work2);
	INIT_WORK(&context->rmnet_read_response_work, rmnet_read_response_work);
	INIT_WORK(&context->rmnet_txpath_work, rmnet_txpath_work);
	
	hrtimer_init(&context->tx_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	context->tx_timer.function = &rmnet_tx_timer_cb;
	context->tx_skb = NULL;
	context->rem_skb = NULL;
	spin_lock_init(&context->tx_lock);
	mutex_init(&context->cxt_lock);

#ifdef QMI_PRESET_QMAP
	context->timer_interval = RMNET_TIMER_INTERVAL_USEC * NSEC_PER_USEC;
	context->tx_max_datagrams = UL_DATA_AGGREGATION_MAX_DATAGRAMS;
	context->tx_max_size = UL_DATA_AGGREGATION_MAX_SIZE;
	context->rx_max_datagrams = DL_DATA_AGGREGATION_MAX_DATAGRAMS;
	context->rx_max_size = DL_DATA_AGGREGATION_MAX_SIZE;
#endif


	if(g_dev_instance_map[real_dev_idx].mux_dev_num == 1){
		// create charactor device
		status = rmnet_create_device(dev);

		if (status < 0) {
			pr_err(NET_DEVICE_NAME " create character device error %d\n", status);
			goto error;
		}else{
			context->is_make_char_dev = true;
		}
	}

	// initialize rmnet context
	context->rmnet_status = 0;
	context->qmi_ctl_txid = QMI_CTL_MIN_TXID;
	context->qmi_wda_txid = QMI_SVC_MIN_TXID;
	context->qmi_wds_txid = QMI_SVC_MIN_TXID;
	context->qmi_wda_client_id = -1;
	context->qmi_wds_client_id = -1;
	context->qmi_wds_client_ex = -1;
	context->qmi_dms_clinet_id = -1;
	context->qmi_nas_client_id = -1;
	context->connect_v4 = CONNECTION_DISCONNECTED;
	context->connect_v6 = CONNECTION_DISCONNECTED;

	// if modify network interface name, uncomment follow line.
	strcpy(dev->net->name, NET_DEVICE_NAME "%d");

	if (context->rx_max_size > 0) dev->rx_urb_size = context->rx_max_size;

	//g_device_contexts[context->dev_instance] = (void *)context;

	pr_info(NET_DEVICE_NAME " bind success\n");

	return 0;

error :
	
	if(real_dev_idx >= 0){
		cleanup_mux_dev(real_dev_idx, dev);
	}
	usb_set_intfdata(context->intf, NULL);
	rmnet_free(context);
	
	dev->data[0] = 0;

	pr_err(NET_DEVICE_NAME " bind error\n");

	return -ENODEV;
}

/*
 * unbind handler
 *
 * Parameters
 *   dev : net device object
 *   intf : interface descriptor
 *
 * Return value
 *   NONE
 */
void rmnet_unbind(struct usbnet *dev, struct usb_interface *intf)
{
	struct rmnet_context *context = (struct rmnet_context *)dev->data[0];

	if (context == NULL) {
		pr_warn(NET_DEVICE_NAME " unbind end - context is null\n");
		return;
	}

	context->rmnet_status |= RMNET_STATUS_REMOVED;
	
	if (hrtimer_active(&context->tx_timer))
		hrtimer_cancel(&context->tx_timer);

	flush_scheduled_work();

	
	rmnet_destroy_device(dev);

	//rmnet_terminate(context);
	rmnet_status_stop(context);
	//usb_set_intfdata(context->intf, NULL);
	rmnet_free(context);

	

	pr_info(NET_DEVICE_NAME " unbind\n");
}

/*
 * status handler
 *
 * Parameters
 *   dev : net device object
 *   urb : interrupt urb
 *
 * Return value
 *   NONE
 */
void rmnet_status(struct usbnet *dev, struct urb *urb)
{
	struct usb_cdc_notification	*event;
	struct rmnet_context *context = (struct rmnet_context *)dev->data[0];

	if (urb->actual_length < sizeof *event) {
		pr_warn(NET_DEVICE_NAME " not enough data packet\n");
		return;
	}

	event = urb->transfer_buffer;
	switch (event->bNotificationType) {
	case USB_CDC_NOTIFY_NETWORK_CONNECTION:
		pr_info(NET_DEVICE_NAME " carrier notification %s\n", event->wValue ? "on" : "off");
		if (event->wValue)
			netif_carrier_on(context->netdev);
		else{
			memset(&context->netdev->stats, 0, sizeof(struct net_device_stats));
			netif_carrier_off(context->netdev);
		}
		break;

	case USB_CDC_NOTIFY_SPEED_CHANGE:	/* tx/rx rates */
		pr_info(NET_DEVICE_NAME " speed change (len %d)\n", urb->actual_length);
		if (urb->actual_length != (sizeof *event + 8))
			set_bit(EVENT_STS_SPLIT, &dev->flags);
		else
			dumpspeed(dev, (__le32 *) &event[1]);
		break;

	case USB_CDC_NOTIFY_RESPONSE_AVAILABLE:
		pr_debug(NET_DEVICE_NAME " read data available\n");
		rmnet_read_response(dev);
		break;

	default:
		pr_warn(NET_DEVICE_NAME " unexpected notification %02x!\n", event->bNotificationType);
		break;
	}
}

/*
 * interrupt complete callback
 *
 * Parameters
 *   urb : interrupt urb
 *
 * Return value
 *   NONE
 */
void rmnet_status_complete(struct urb *urb)
{
	struct rmnet_context *context = urb->context;
	int	status = urb->status;
	
	pr_debug(NET_DEVICE_NAME " interrupt complete status %d\n", status);

	switch (status) {
	case 0:
		rmnet_status(context->dev, urb);
		break;

	case -ENOENT:		/* urb killed */
	case -ESHUTDOWN:	/* hardware gone */
		return;
	
	default:
		break;
	}

	//if (!netif_running (context->netdev))
	//	return;

	memset(urb->transfer_buffer, 0, urb->transfer_buffer_length);
	status = usb_submit_urb(urb, GFP_ATOMIC);
}

/*
 * start interrupt
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   = 0 : success
 *   < 0 : error
 */
int rmnet_status_start(struct rmnet_context *context)
{
	unsigned	pipe = 0;
	unsigned	maxp;
	unsigned	period;

	pr_info(NET_DEVICE_NAME " status start\n");

	if (!context->ep_interrupt) {
		pr_err(NET_DEVICE_NAME " there is no interrupt pipe\n");
		return -ENODEV;
	}
	
	if (context->interrupt_urb) {
		return -EIO;
	}

	pipe = usb_rcvintpipe(context->udev,
			context->ep_interrupt->desc.bEndpointAddress
				& USB_ENDPOINT_NUMBER_MASK);
	maxp = usb_maxpacket(context->udev, pipe, 0);

	/* avoid 1 msec chatter:  min 8 msec poll rate */
	period = max((int) context->ep_interrupt->desc.bInterval,
		(context->udev->speed == USB_SPEED_HIGH) ? 7 : 3);
	
	context->interrupt_urb = usb_alloc_urb (0, GFP_KERNEL);
	if (!context->interrupt_urb) {
		pr_err(NET_DEVICE_NAME " urb allocation error\n");
		return -ENOMEM;
	}
	
	context->interrupt_buf = kmalloc(maxp, GFP_KERNEL);
	if (!context->interrupt_buf) {
		pr_err(NET_DEVICE_NAME " interrupt buffer allocation errer\n");
		usb_free_urb(context->interrupt_urb);
		context->interrupt_urb = NULL;
		return -ENOMEM;
	}
	
	usb_fill_int_urb(context->interrupt_urb, 
	                 context->udev, 
	                 pipe,
	                 context->interrupt_buf, 
	                 maxp, 
	                 rmnet_status_complete, 
	                 context, 
	                 period);

	atomic_set(&g_dev_instance_map[context->real_dev_idx].cdc_noti_read_count, 0);
	
	return usb_submit_urb(context->interrupt_urb, GFP_KERNEL);
}

/*
 * stop interrupt
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   NONE
 */
void rmnet_status_stop(struct rmnet_context *context)
{
	pr_info(NET_DEVICE_NAME " status stop\n");
	
	if (context->interrupt_urb) {
		usb_kill_urb(context->interrupt_urb);
	}
	
	if (context->interrupt_buf) {
		kfree(context->interrupt_buf);
		context->interrupt_buf = NULL;
	}
	
	if (context->interrupt_urb) {
		usb_free_urb(context->interrupt_urb);
		context->interrupt_urb = NULL;
	}
}

/*
 * make qmap packet
 *
 * Parameters
 *   dev : net device object
 *   skb : received packet
 *
 * Return value
 *   socket buffer
 */
struct sk_buff *rmnet_tx_fill_frame(struct usbnet *dev, struct sk_buff *skb)
{
	struct sk_buff *skb_out;
	struct qmap_header *qmap_hdr;
	struct rmnet_context *context = (struct rmnet_context *)dev->data[0];
	u16 n = 0, data_len;
	u8 padding, send_ok = 0;
	
	if (skb != NULL) {
		skb_pull(skb, ETH_HLEN);
		swap(skb, context->rem_skb);
	} else {
		send_ok = 1;
	}
	
	skb_out = context->tx_skb;
	
	if (!skb_out) {
		skb_out = alloc_skb(context->tx_max_size, GFP_ATOMIC);
		if (skb_out == NULL) {
			if (skb != NULL) {
				dev_kfree_skb_any(skb);
				dev->net->stats.tx_dropped++;
			}
			goto exit_no_skb;
		}
		
		context->tx_datagrams = 0;
		context->tx_payloads = 0;
	}
	
	for (n = context->tx_datagrams; n < context->tx_max_datagrams; n++) {
		if (skb == NULL) {
			skb = context->rem_skb;
			context->rem_skb = NULL;
			
			if (skb == NULL) {
				break;
			}
		}
		
		padding = skb->len % 4;
		padding = padding > 0 ? 4 - padding : 0;
		data_len = padding + skb->len;
		
		if (context->tx_max_size < skb_out->len + sizeof(struct qmap_header) + data_len) {
			if (n == 0) {
				dev_kfree_skb_any(skb);
				skb = NULL;
				dev->net->stats.tx_dropped++;
			} else {
				if (context->rem_skb != NULL) {
					dev_kfree_skb_any(context->rem_skb);
					dev->net->stats.tx_dropped++;
				}
				context->rem_skb = skb;
				skb = NULL;
				send_ok = 1;
			}
			break;
		}
		
		qmap_hdr = (struct qmap_header *)skb_put(skb_out, sizeof(struct qmap_header));
		qmap_hdr->padding = padding;

		qmap_hdr->mux_id = context->mux_id;
		
		//printk(KERN_WARNING "[write]rmnet_tx_fixup_multi - muxid is chk [muxid is : 0x%x]\n", qmap_hdr->mux_id);
		
		qmap_hdr->data_len = htons(data_len);
		memcpy(skb_put(skb_out, skb->len), skb->data, skb->len);
		if (padding > 0) memset(skb_put(skb_out, padding), 0, padding);
		context->tx_payloads += sizeof(struct qmap_header) + data_len;
		dev_kfree_skb_any(skb);
		skb = NULL;
	}
	
	if (skb != NULL) {
		dev_kfree_skb_any(skb);
		skb = NULL;
		dev->net->stats.tx_dropped++;
	}
	
	context->tx_datagrams = n;
	
	if (n == 0) {
		context->tx_skb = skb_out;
		goto exit_no_skb;
	} else if ((n < context->tx_max_datagrams) && (send_ok == 0) && (context->timer_interval > 0)) {
		context->tx_skb = skb_out;
		if (n < RMNET_RESTART_TIMER_DATAGRAM_CNT)
			context->tx_timer_pending = RMNET_TIMER_PENDING_CNT;
		goto exit_no_skb;
	} else if (atomic_read(&context->tx_flow_stop) == 1) {
		context->tx_skb = skb_out;
		goto exit_no_skb;
	}
	
	context->tx_skb = NULL;
	dev->net->stats.tx_packets += context->tx_datagrams;
	return skb_out;
	
exit_no_skb :
	if (context->tx_skb != NULL && n > 0) {
		rmnet_tx_timeout_start(context);
	}

	return NULL;
}

/*
 * received packet process handler
 *
 * Parameters
 *   dev : net device object
 *   skb : received packet
 *
 * Return value
 *   > 0 : error
 *   = 0 : success
 */
int rmnet_rx_fixup(struct usbnet *dev, struct sk_buff *skb)
{
	PRINT_DEBUG(KERN_DEBUG "%s : rx packet fixup\n", NET_DEVICE_NAME);
	if (skb_headroom(skb) >= ETH_HLEN) {
		skb_push(skb, ETH_HLEN);
		skb_reset_mac_header(skb);
		eth_hdr(skb)->h_proto = htons(ETH_P_IP);
		memset(eth_hdr(skb)->h_source, 0, ETH_ALEN);
		memcpy(eth_hdr(skb)->h_dest, dev->net->dev_addr, ETH_ALEN);
	} else if (skb_tailroom(skb) >= ETH_HLEN) {
		skb_put(skb, ETH_HLEN);
		memmove(skb->data + ETH_HLEN, skb->data, skb->len);
		skb_reset_mac_header(skb);
		eth_hdr(skb)->h_proto = htons(ETH_P_IP);
		memset(eth_hdr(skb)->h_source, 0, ETH_ALEN);
		memcpy(eth_hdr(skb)->h_dest, dev->net->dev_addr, ETH_ALEN);
	} else {
		pr_err(NET_DEVICE_NAME " fixup error : not enough headroom\n");
		return 0;
	}
	return 1;
}

/*
 * received packet process handler for multi packets
 *
 * Parameters
 *   dev : net device object
 *   skb : received packet
 *
 * Return value
 *   > 0 : error
 *   = 0 : success
 */
int rmnet_rx_fixup_multi(struct usbnet *dev, struct sk_buff *skb_in)
{
	//struct rmnet_context *context = (struct rmnet_context *)dev->data[0];
	int padding, datagram_len, block_len;
	int skb_total_len, data_offset;
	struct qmap_header *qmap_hdr = NULL;
	struct qmap_control *qmap_ctl = NULL;
	struct sk_buff *skb = NULL;
	__be16 proto = 0;
	unsigned char ip_version = 0;
	int status;

	struct rmnet_context* real_context = NULL;
	struct rmnet_context* context = (struct rmnet_context*) dev->data[0];
	struct usbnet* real_dev = NULL;

	data_offset = 0;
	block_len = 0;
	skb_total_len = skb_in->len;
	
	while (skb_total_len > data_offset) {
				
		qmap_hdr = (struct qmap_header *)(skb_in->data + data_offset);

		if(ntohs(qmap_hdr->data_len) == 0){
			break;
		}
				
		padding = (qmap_hdr->padding & 0x3F);
		datagram_len = ntohs(qmap_hdr->data_len)- padding;
		block_len = sizeof(struct qmap_header) + ntohs(qmap_hdr->data_len);

		if(ntohs(qmap_hdr->data_len) == 0){
			break;
		}
		
		status = find_rmnet_context_with_mux_id(qmap_hdr->mux_id, &real_context, context->real_dev_idx);
		
		if(status < 0){
			printk(KERN_WARNING "rmnet_rx_fixup_multi - find real rmnet_context fail [dump : 0x%02x 0x%02x 0x%04x] [offset : %d]\n", qmap_hdr->padding, qmap_hdr->mux_id, qmap_hdr->data_len, data_offset);
			goto error;
		}else{
			real_dev = real_context->dev;
		}
		
		if ((qmap_hdr->padding & 0x80) > 0) {
			
			qmap_ctl = (struct qmap_control *)(skb_in->data + data_offset + sizeof(struct qmap_header));

			pr_info(NET_DEVICE_NAME " qmap control command : %x\n", qmap_ctl->command_name);
			
			if (qmap_ctl->command_name == 1) {
				atomic_set(&real_context->tx_flow_stop, 1);
			} else {
				atomic_set(&real_context->tx_flow_stop, 0);
			}
						
		} else {
			if(qmap_hdr->mux_id == real_context->mux_id){
				ip_version = *(skb_in->data + data_offset + sizeof(struct qmap_header));
				switch (ip_version & 0xf0) {
				case 0x40:
					proto = htons(ETH_P_IP);
					break;
				case 0x60:
					proto = htons(ETH_P_IPV6);
					break;
				default:
					proto = 0;
				}
				
				if (proto != 0) {
					//skb = netdev_alloc_skb_ip_align(dev->net, datagram_len + ETH_HLEN);
					skb = netdev_alloc_skb_ip_align(real_dev->net, datagram_len + ETH_HLEN);
					if (!skb) {
						goto error;
					}
					
					skb_put(skb, datagram_len + ETH_HLEN);
					skb_reset_mac_header(skb);
					eth_hdr(skb)->h_proto = proto;
					memset(eth_hdr(skb)->h_source, 0, ETH_ALEN);
					//memcpy(eth_hdr(skb)->h_dest, dev->net->dev_addr, ETH_ALEN);
					memcpy(eth_hdr(skb)->h_dest, real_dev->net->dev_addr, ETH_ALEN);
					memcpy(skb->data + ETH_HLEN, skb_in->data + data_offset + sizeof(struct qmap_header), datagram_len);
					//usbnet_skb_return(dev, skb);
					usbnet_skb_return(real_dev, skb);
				}
			}else{
				printk(KERN_WARNING "rmnet_rx_fixup_multi - muxid is invalid [muxid is : 0x%x]\n", qmap_hdr->mux_id);
			}
		}
		data_offset += block_len;
	}
	return 1;
error :
	return 0;
}

/*
 * transmit packet process handler
 *
 * Parameters
 *   dev : net device object
 *   skb : received packet
 *   flags : buffer flags
 *
 * Return value
 *   socket buffer
 */
struct sk_buff *rmnet_tx_fixup(struct usbnet *dev, struct sk_buff *skb, gfp_t flags)
{
	struct rmnet_context *context = (struct rmnet_context *)dev->data[0];
	struct iphdr *ip_header = NULL;
	int padding, data_len;
	struct qmap_header *qmap_hdr;
	
	if (context == NULL) {
		pr_err(NET_DEVICE_NAME " tx_fixup error - no context data\n");
		goto error;
	}

	PRINT_DEBUG(KERN_DEBUG "%s : tx packet fixup\n", NET_DEVICE_NAME);
	
	padding = 0;
	data_len = 0;
	qmap_hdr = NULL;

	skb_pull(skb, ETH_HLEN);

	ip_header = (struct iphdr *)skb->data;
	if (ip_header->version == IPVERSION) {
		if (ip_header->saddr != 0 &&
			ip_header->saddr != context->ipv4_address) {
			pr_err(NET_DEVICE_NAME " packet droped : ip address mismatch - packet(%x):adapter(%x)\n", 
				ip_header->saddr, context->ipv4_address);
			goto error;
		}
	}

#ifdef QMI_PRESET_QMAP
	padding = skb->len % 4;
	if (padding > 0) padding = 4 - padding;
	data_len = skb->len;
	skb_put(skb, padding);
	memset(skb->data + data_len, 0, padding);
	skb_push(skb, sizeof(struct qmap_header));
	qmap_hdr = (struct qmap_header *)skb->data;
	qmap_hdr->padding = padding;
	qmap_hdr->mux_id = context->mux_id;
	qmap_hdr->data_len = htons(data_len + padding);
#endif
	return skb;

error:
	if (skb != NULL)
		dev_kfree_skb_any(skb);

	return NULL;
}

/*
 * transmit packet process handler for multi packets
 *
 * Parameters
 *   dev : net device object
 *   skb : received packet
 *   flags : buffer flags
 *
 * Return value
 *   socket buffer
 */
struct sk_buff *rmnet_tx_fixup_multi(struct usbnet *dev, struct sk_buff *skb, gfp_t flags)
{
	struct sk_buff *skb_out;
	struct rmnet_context *context = (struct rmnet_context *)dev->data[0];
	
	if (context == NULL) {
		pr_err(NET_DEVICE_NAME " tx_fixup error - no context data\n");
		goto error;
	}
	
	spin_lock_bh(&context->tx_lock);
	skb_out = rmnet_tx_fill_frame(dev, skb);
	spin_unlock_bh(&context->tx_lock);
	
	return skb_out;

error:
	if (skb != NULL)
		dev_kfree_skb_any(skb);

	return NULL;
}

static const struct driver_info	rmnet_info = {
	.description	= NET_MODULE_DESC,
#ifdef QMI_PRESET_QMAP
	.flags			= FLAG_ETHER | FLAG_POINTTOPOINT | FLAG_SEND_ZLP | FLAG_MULTI_PACKET,
	.rx_fixup		= rmnet_rx_fixup_multi,
	.tx_fixup		= rmnet_tx_fixup_multi,
#else
	.flags			= FLAG_ETHER | FLAG_POINTTOPOINT | FLAG_SEND_ZLP,
	.rx_fixup		= rmnet_rx_fixup,
	.tx_fixup		= rmnet_tx_fixup,
#endif
	.bind			= rmnet_bind,
	.unbind			= rmnet_unbind,
	.status			= rmnet_status,
};

/*-------------------------------------------------------------------------*/

/*
 * probe handler
 *
 * Parameters
 *   udev : usb interface object
 *   prod : 
 *
 * Return value
 *   < 0 : error
 *   = 0 : success
 */
int rmnet_probe(struct usb_interface *intf, const struct usb_device_id *prod)
{
	int status = 0;
	int i = 0;
	int real_dev_idx;

#ifdef NET_INTERFACE_NUM
	if (intf->cur_altsetting->desc.bInterfaceNumber != NET_INTERFACE_NUM) {
		pr_err(NET_DEVICE_NAME " no matched interface(%d)\n", intf->cur_altsetting->desc.bInterfaceNumber);
		return -ENODEV;
	}
#endif

	// call general function
	pr_info(NET_DEVICE_NAME " prod->idVendor %x\n", (unsigned int)prod->idVendor);
	pr_info(NET_DEVICE_NAME " prod->idProduct %x\n", (unsigned int)prod->idProduct);
	pr_info(NET_DEVICE_NAME " prod->driver_info %x\n", (unsigned int)prod->driver_info);

    switch((unsigned int)prod->idProduct) {
        case 0x3100:
            rmnet_mux_device_num = 1;
            break;

        case 0x3102:
            rmnet_mux_device_num = 2;
            break;

        case 0x3103:
            rmnet_mux_device_num = 3;
            break;

        case 0x3104:
            rmnet_mux_device_num = 4;
            break;

        default:
            rmnet_mux_device_num = 1;
            break;
    }

	printk("rmnet_mux_device_num = %d\n", rmnet_mux_device_num);
	
	status = add_real_dev(intf);

	if(status < 0){
		printk(KERN_ERR "rmnet_probe - cannot register usb_interface current connected deivces %d\n", real_dev_num);
		goto EXIT;
	}

	for(i = 0; i < rmnet_mux_device_num; i++){
		
		status = usbnet_probe(intf, prod);

		printk(KERN_NOTICE "probe from %s(pid %d) status %d\n", current->comm, current->pid, status);
		
		if (status == 0) {
			struct usbnet *dev = usb_get_intfdata(intf);
			struct rmnet_context *context;
			context = (struct rmnet_context *)dev->data[0];

			if(i == 0){
				
				status = rmnet_set_line_state(context);
				
				if (status < 0) {
					printk(KERN_WARNING "set line state error : %d\n", status);
				}				
			}

			status = rmnet_status_start(context);
			
			if (status < 0) {
				printk(KERN_WARNING "status start error : %d\n", status);
				goto EXIT;

			}
			
		}else{
			printk(KERN_WARNING "status start error : %d\n", status);
			goto EXIT;
		}
		
	}
	
	real_dev_idx = find_real_dev(intf);

	if(real_dev_idx < 0){
		status = -ENODEV;
		goto EXIT;
	}

	if(schedule_work(&g_dev_instance_map[real_dev_idx].qmi_init_work) == 0){
		pr_err(NET_DEVICE_NAME " schedule qmi init work fail.\n");
		status = -EBUSY;
		goto EXIT;
	}


EXIT:

	if(status < 0){
		
		real_dev_idx = find_real_dev(intf);

		if(real_dev_idx >= 0){
			cleanup_real_dev(real_dev_idx);
		}
	}
	
	return status;
}

/*
 * disconnect handler
 *
 * Parameters
 *   intf : usb interface object
 *
 * Return value
 *   NONE
 */
void rmnet_disconnect(struct usb_interface *intf)
{
	struct usbnet		*dev;
	struct usb_device	*xdev;
	struct net_device	*net;
	int i;
	int real_dev_idx = find_real_dev(intf);

	if(real_dev_idx < 0){
		printk(KERN_ERR "rmnet_disconnect - find real_dev idx fail\n");
		return;
	}

	set_remove_pending(real_dev_idx, 1);

	flush_scheduled_work();

	for(i = 0; i < rmnet_mux_device_num; i++){
		rmnet_terminate((struct rmnet_context *)g_dev_instance_map[real_dev_idx].mux_dev_list[i]->data[0]);
	}
	
	for(i = 0; i < rmnet_mux_device_num; i++){

		dev = g_dev_instance_map[real_dev_idx].mux_dev_list[i];
		
		if(!dev){
			printk(KERN_ERR "rmnet_disconnect - mux_dev idx %d is null.. skip\n", i);
			continue;
		}

		if (dev->driver_info->unbind)
			dev->driver_info->unbind (dev, intf);

		xdev = interface_to_usbdev (intf);
		
		netif_info(dev, probe, dev->net, "unregister '%s' usb-%s-%s, %s\n",
			   intf->dev.driver->name,
			   xdev->bus->bus_name, xdev->devpath,
			   dev->driver_info->description);
		
		net = dev->net;
		
		unregister_netdev (net);
		cancel_work_sync(&dev->kevent);
		usb_scuttle_anchored_urbs(&dev->deferred);
		usb_kill_urb(dev->interrupt);
		usb_free_urb(dev->interrupt);
		kfree(dev->padding_pkt);
		free_netdev(net);
		
	}
	
	usb_set_intfdata(intf, NULL);
	cleanup_real_dev(real_dev_idx);
	
}



/*
 * suspend handler
 *
 * Parameters
 *   intf : usb interface object
 *   message : suspend parameter
 *
 * Return value
 *   = 0 : success
 *   < 0 : error
 */
int rmnet_suspend (struct usb_interface *intf, pm_message_t message)
{
	struct usbnet *dev = usb_get_intfdata(intf);
	struct rmnet_context *context;
	context = (struct rmnet_context *)dev->data[0];

	rmnet_status_stop(context);

	return usbnet_suspend(intf, message);
}

/*
 * resume handler
 *
 * Parameters
 *   intf : usb interface object
 *
 * Return value
 *   = 0 : success
 *   < 0 : error
 */
int rmnet_resume (struct usb_interface *intf)
{
	struct usbnet *dev = usb_get_intfdata(intf);
	struct rmnet_context *context;
	int status;
	context = (struct rmnet_context *)dev->data[0];

	status = rmnet_status_start(context);
	if (status < 0) {
		pr_warn(NET_DEVICE_NAME " status start error : %d\n", status);
	}

	return usbnet_resume(intf);
}

static const struct usb_device_id	rmnet_devices[] = {
{
	DEVICE_MATCH_INFO_3100,
	.driver_info = (unsigned long)&rmnet_info,
},
{
    DEVICE_MATCH_INFO_3102,
    .driver_info = (unsigned long)&rmnet_info,
},
{
    DEVICE_MATCH_INFO_3103,
    .driver_info = (unsigned long)&rmnet_info,
},
{
    DEVICE_MATCH_INFO_3104,
    .driver_info = (unsigned long)&rmnet_info,
},
{
	// END
},
};

MODULE_DEVICE_TABLE(usb, rmnet_devices);

static struct usb_driver rmnet_driver = {
	.name 			="rmnet_ether",
	.id_table 		= rmnet_devices,
	.probe 			= rmnet_probe,
	.disconnect 	= rmnet_disconnect,
	.suspend 		= rmnet_suspend,
	.resume 		= rmnet_resume,
	.reset_resume 	= rmnet_resume,
	.supports_autosuspend = 1,
};

/*
 * module init
 *
 * Parameters
 *   NONE
 *
 * Return value
 *   < 0 : error
 *   = 0 : success
 */
static int __init rmnet_init(void)
{
	int err = 0;
	int i, j;

	rmnet_service_class = class_create(THIS_MODULE, CHAR_DEVICE_NAME);
	err = PTR_ERR(rmnet_service_class);
	if (IS_ERR(rmnet_service_class)) {
		printk(KERN_ERR "%s : class create failed 0x%08x\n", CHAR_DEVICE_NAME, err);
		goto error;
	}

	printk(KERN_ERR "%s : rmnet_init start (%s)\n", NET_DEVICE_NAME, DRIVER_VERSION);

	
	for(i = 0 ; i < MAX_CONNECT_DEVICES; i++){
		
		g_dev_instance_map[i].real_dev = NULL;
		
		for(j = 0 ; j < MAX_RMNET_MUX_DEVICE_NUM; j++){
			
			g_dev_instance_map[i].mux_dev_list[j] = NULL;
		}
		
		g_dev_instance_map[i].mux_dev_num = 0;
		g_dev_instance_map[i].qmi_clt_txid = QMI_CTL_MIN_TXID;
	}
	
	real_dev_num = 0;
	

	printk(KERN_NOTICE "%s : driver module init (%s)\n", NET_DEVICE_NAME, DRIVER_VERSION);
	
 	return usb_register(&rmnet_driver);

error :

	printk(KERN_ERR "%s : driver module init failure\n", NET_DEVICE_NAME);

	return err;
}
module_init(rmnet_init);

/*
 * module exit
 *
 * Parameters
 *   NONE
 *
 * Return value
 *   NONE
 */
static void __exit rmnet_exit(void)
{
 	usb_deregister(&rmnet_driver);

	class_destroy(rmnet_service_class);

	printk(KERN_NOTICE "%s : driver module exit\n", NET_DEVICE_NAME);
}
module_exit(rmnet_exit);

MODULE_AUTHOR("Lee Jinho");
MODULE_DESCRIPTION(NET_MODULE_DESC);
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
