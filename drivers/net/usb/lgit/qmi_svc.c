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
#include <linux/fs.h>

#include "config.h"
#include "util.h"
#include "rmnet.h"
#include "qmi_ctl.h"
#include "qmi_svc.h"

#include "qmi_svc_wda.h"
#include "qmi_svc_wds.h"

/*
 * Get next transaction id and increase next transaction id
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   transaction id
 */
int qmi_svc_get_txid(int *txid)
{
	// get transaction id and increase
	*txid = (*txid)+1;

	// if transaction id is max value, reset transaction id
	if (*txid == QMI_SVC_MAX_TXID) {
		*txid = QMI_SVC_MIN_TXID;
	}

	return *txid;
}

/*
 * Send qmi service message
 *
 * Parameters
 *   context : rmnet context
 *   msg : control message
 *   msglen : control message length
 *
 * Return value
 *   < 0 : message send failure
 *   other : message send success
 */
int qmi_svc_send_and_forget(struct rmnet_context *context, void *msg, int msglen)
{
	int status = 0;
	u8 remove_pending = 0;

	pr_debug(NET_DEVICE_NAME " send and forget control message\n");

	if(context == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_send_and_forget - device context is null - return\n");
		return -EINVAL;
	}

	if(get_remove_pending(context->real_dev_idx, &remove_pending) < 0){
		pr_warn(NET_DEVICE_NAME " qmi_svc_send_and_forget - remove pending check - return\n");
		return -ENODEV;
	}

	if(remove_pending == 1){
		pr_warn(NET_DEVICE_NAME " qmi_svc_send_and_forget - remove pending condition - return\n");
		return -ENODEV;		
	}

	if (RMNET_STATUS_REMOVED & context->rmnet_status) {
		pr_warn(NET_DEVICE_NAME " qmi_svc_send_and_forget : device already removed\n");
		return -ENODEV;
	}

	if(get_real_dev(context->real_dev_idx) == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_send_and_forget - device is null - return\n");
		return -EINVAL;
	}

	
	DBG_DUMP_MEMORY((char *)msg, msglen);

	// send qmi service message
	status = usb_control_msg(context->udev,
							 usb_sndctrlpipe(context->udev, 0),
							 USB_CDC_SEND_ENCAPSULATED_COMMAND,
							 USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
							 0,
							 context->intf->cur_altsetting->desc.bInterfaceNumber,
							 msg,
							 msglen,
							 RMNET_RESPONSE_TIMEOUT_MS);
							 
	// service message send failure
	if (status < 0) {
		pr_err(NET_DEVICE_NAME " usb service message send error %d\n", status);
	}

	return status;
}

/*
 * Send service message and wait response
 *
 * Parameters
 *   context : rmnet context
 *   msg : control message
 *   msglen : control message length
 *   txid : transaction id
 *   buf : buffer for response message
 *   buflen : maximum length for response message buffer
 * 
 * Return value
 *   < 0 : message send failure
 *   = 0 : message send ok but no response
 *   > 0 : message send success and response ok
 */
int qmi_svc_send_and_wait(struct rmnet_context *context, u8 service_type, u8 client_id, void *msg, int msglen, u8 txid, void *buf, int buflen)
{
	int status = 0;

	struct qmi_ctl_transaction_item *tx_item = NULL;
	u8 remove_pending = 0;

	DECLARE_WAIT_QUEUE_HEAD(wait_queue_head);

	if(context == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_send_and_wait - device context is null - return\n");
		return -EINVAL;
	}

	if(get_remove_pending(context->real_dev_idx, &remove_pending) < 0){
		pr_warn(NET_DEVICE_NAME " qmi_svc_send_and_wait - remove pending check - return\n");
		return -ENODEV;
	}

	if(remove_pending == 1){
		pr_warn(NET_DEVICE_NAME " qmi_svc_send_and_wait - remove pending condition - return\n");
		return -ENODEV;		
	}

	if (RMNET_STATUS_REMOVED & context->rmnet_status) {
		pr_warn(NET_DEVICE_NAME " qmi_svc_send_and_wait : device already removed\n");
		return -ENODEV;
	}

	if(get_real_dev(context->real_dev_idx) == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_send_and_wait - device is null - return\n");
		return -EINVAL;
	}

	// allocate memory for transaction item
	tx_item = kmalloc(sizeof(struct qmi_ctl_transaction_item), GFP_KERNEL);
	if (tx_item == NULL) {
		pr_err(NET_DEVICE_NAME " service txitem memory alloc error\n");
		status = -ENOMEM;
		goto exit;
	}

	// initialize transaction item
	tx_item->wait_queue_head = &wait_queue_head;
	tx_item->service_type = service_type;
	tx_item->client_id = client_id;
	tx_item->transaction_id = txid;
	tx_item->buffer = buf;
	tx_item->length = 0;
	tx_item->qmi_result = 0;
	tx_item->qmi_error = 0;

	// append transaction item
	mutex_lock(&context->cxt_lock);
	add_list_tail(&context->tx_list_head, &tx_item->list);
	mutex_unlock(&context->cxt_lock);

	pr_debug(NET_DEVICE_NAME " send and wait message(txid:0x%x)\n", txid);
	DBG_DUMP_MEMORY((char *)msg, msglen);

	// send service message
	status = usb_control_msg(context->udev,
							 usb_sndctrlpipe(context->udev, 0),
							 USB_CDC_SEND_ENCAPSULATED_COMMAND,
							 USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
							 0,
							 context->intf->cur_altsetting->desc.bInterfaceNumber,
							 msg,
							 msglen,
							 RMNET_RESPONSE_TIMEOUT_MS);

	// service message send failure
	if (status < 0) {
		pr_err(NET_DEVICE_NAME " usb service message send error %d\n", status);
        mutex_lock(&context->cxt_lock);
		remove_list(&tx_item->list);
        mutex_unlock(&context->cxt_lock);
		goto exit;
	}

	// sleep few seconds for response
	wait_event_interruptible_timeout(wait_queue_head, tx_item->length != 0, 5*HZ);

	// remove transaction item in list
	mutex_lock(&context->cxt_lock);
	remove_list(&tx_item->list);
	mutex_unlock(&context->cxt_lock);

	pr_debug(NET_DEVICE_NAME " wakeup from sleep - buffer received %d bytes\n", tx_item->length);

	// set status
	status = tx_item->length;

exit :

	// free transaction item
	if (tx_item)
		kfree(tx_item);

	return status;
}

/*
 * Find tlv in response message
 *
 * Parameters
 *   tlv : start pointer of tlvs
 *   type : find type of tlv
 *   length : source tlvs length
 *
 * Return value
 *   pointer of found tlv
 */
unsigned char *qmi_svc_find_tlv(unsigned char *tlv, u8 type, u16 length)
{
	u16 i;
	int found = 0;
	struct qmi_svc_common_tlv *common_tlv = NULL;

	// retreive tlvs
	for (i = 0 ; i < length ; ) {
		common_tlv = (struct qmi_svc_common_tlv *)&tlv[i];

		// found tlv
		if (common_tlv->type == type) {
			found = 1;
			break;
		}

		// move offset
		i += (common_tlv->length + 3);
	}

	// if not found tlv, return value is NULL
	if (found == 0) common_tlv = NULL;

	return (unsigned char *)common_tlv;
}

/*
 * QMI_WDA_SET_DATA_FORMAT
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   < 0 : failure reason ERROR
 *   = 0 : failure reason TIMEOUT
 *   > 0 : success
 */
int qmi_wda_set_data_format(struct rmnet_context *context)
{
	int status = 0, request_size = 0;
	char *buffer = NULL, *req_offset = NULL, *read_offset = NULL;
	u16 transaction_id = 0;
	struct qmi_wda_set_data_format_req *msg = NULL;
	struct qmi_wda_qos_data_format *qos_data_format;
	struct qmi_wda_link_layer_protocol *link_layer_prot;
	struct qmi_wda_data_aggregation_protocol *data_prot;
	struct qmi_wda_data_aggregation_max_datagrams *max_datagrams;
	struct qmi_wda_data_aggregation_max_size *data_max_size;
	struct qmi_wda_peripheral_endpoint_type *ep_type;
	struct qmi_svc_response_header *response = NULL;
	
	qos_data_format = NULL;
	link_layer_prot = NULL;
	data_prot = NULL;
	max_datagrams = NULL;
	data_max_size = NULL;
	ep_type = NULL;

	pr_debug(NET_DEVICE_NAME " qmi wda set data format\n");
	
	request_size = sizeof(struct qmi_wda_set_data_format_req);
#ifdef QOS_DATA_FORMAT
	request_size += sizeof(struct qmi_wda_qos_data_format);
#endif
#ifdef LINK_LAYER_PROTOCOL
	request_size += sizeof(struct qmi_wda_link_layer_protocol);
#endif
#ifdef UL_DATA_AGGREGATION_PROT
	request_size += sizeof(struct qmi_wda_data_aggregation_protocol);
#endif
#ifdef DL_DATA_AGGREGATION_PROT
	request_size += sizeof(struct qmi_wda_data_aggregation_protocol);
#endif
#ifdef DL_DATA_AGGREGATION_MAX_DATAGRAMS
	request_size += sizeof(struct qmi_wda_data_aggregation_max_datagrams);
#endif
#ifdef DL_DATA_AGGREGATION_MAX_SIZE
	request_size += sizeof(struct qmi_wda_data_aggregation_max_size);
#endif
#ifdef PERIPHERAL_EP_ID_EP_TYPE
#ifdef PERIPHERAL_EP_ID_IFACE_ID
	request_size += sizeof(struct qmi_wda_peripheral_endpoint_type);
#endif
#endif
	
	if (request_size == sizeof(struct qmi_wda_set_data_format_req)) {
		pr_err(NET_DEVICE_NAME " there is no data format data\n");
		return 0;
	}

	transaction_id = (u16)qmi_svc_get_txid(&context->qmi_wda_txid);

	// allocate memory for send + response message buffer
	buffer = kmalloc(request_size + RMNET_RESPONSE_BUFFER_LEN, GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, request_size);

	// initialize send message
	msg = (struct qmi_wda_set_data_format_req *)buffer;
	msg->message_id = QMI_WDA_SET_DATA_FORMAT;
	msg->length = request_size - sizeof(struct qmi_wda_set_data_format_req);
	req_offset = buffer + sizeof(struct qmi_wda_set_data_format_req);
#ifdef QOS_DATA_FORMAT
	qos_data_format = (struct qmi_wda_qos_data_format *)req_offset;
	qos_data_format->type = 0x10;
	qos_data_format->length = 0x1;
	qos_data_format->qos_format = QOS_DATA_FORMAT;
	req_offset += sizeof(struct qmi_wda_qos_data_format);
#endif
#ifdef LINK_LAYER_PROTOCOL
	link_layer_prot = (struct qmi_wda_link_layer_protocol *)req_offset;
	link_layer_prot->type = 0x11;
	link_layer_prot->length = 0x4;
	link_layer_prot->link_prot = LINK_LAYER_PROTOCOL;
	req_offset += sizeof(struct qmi_wda_link_layer_protocol);
#endif
#ifdef UL_DATA_AGGREGATION_PROT
	data_prot = (struct qmi_wda_data_aggregation_protocol *)req_offset;
	data_prot->type = 0x12;
	data_prot->length = 0x04;
	data_prot->data_protocol = UL_DATA_AGGREGATION_PROT;
	req_offset += sizeof(struct qmi_wda_data_aggregation_protocol);
#endif
#ifdef DL_DATA_AGGREGATION_PROT
	data_prot = (struct qmi_wda_data_aggregation_protocol *)req_offset;
	data_prot->type = 0x13;
	data_prot->length = 0x04;
	data_prot->data_protocol = DL_DATA_AGGREGATION_PROT;
	req_offset += sizeof(struct qmi_wda_data_aggregation_protocol);
#endif
#ifdef DL_DATA_AGGREGATION_MAX_DATAGRAMS
	max_datagrams = (struct qmi_wda_data_aggregation_max_datagrams *)req_offset;
	max_datagrams->type = 0x15;
	max_datagrams->length = 0x04;
	max_datagrams->max_datagrams = DL_DATA_AGGREGATION_MAX_DATAGRAMS;
	req_offset += sizeof(struct qmi_wda_data_aggregation_max_datagrams);
#endif
#ifdef DL_DATA_AGGREGATION_MAX_SIZE
	data_max_size = (struct qmi_wda_data_aggregation_max_size *)req_offset;
	data_max_size->type = 0x16;
	data_max_size->length = 0x04;
	data_max_size->max_size = DL_DATA_AGGREGATION_MAX_SIZE;
	req_offset += sizeof(struct qmi_wda_data_aggregation_max_size);
#endif
#ifdef PERIPHERAL_EP_ID_EP_TYPE
#ifdef PERIPHERAL_EP_ID_IFACE_ID
	ep_type = (struct qmi_wda_peripheral_endpoint_type *)req_offset;
	ep_type->type = 0x17;
	ep_type->length = 0x08;
	ep_type->ep_type = PERIPHERAL_EP_ID_EP_TYPE;
	ep_type->iface_id = PERIPHERAL_EP_ID_IFACE_ID;
	req_offset += sizeof(struct qmi_wda_peripheral_endpoint_type);
#endif
#endif
	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xc + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_WDA;
	msg->msg_header.client_id = context->qmi_wda_client_id;

	read_offset = buffer + request_size;

	// send message and wait response
	status = qmi_svc_send_and_wait(context, 
					RMNET_QMI_VERSION_WDA,
					msg->msg_header.client_id,
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1, // message length + if_type length
					msg->sdu_header.transaction_id,
					read_offset,
					RMNET_RESPONSE_BUFFER_LEN);

	// send failure
	if (status <= 0) {
		pr_err(NET_DEVICE_NAME " qmi wda set data format error\n");
		goto exit;
	}

	// get response message buffer
	response = (struct qmi_svc_response_header *)read_offset;

	// qmi result error
	if (response->qmi_result != 0) {
		pr_err(NET_DEVICE_NAME " qmi wda set data format result error\n");
		status = -1;
		goto exit;
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}

/*
 * QMI_WDA_SET_QMAP_SETTINGS
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   < 0 : failure reason ERROR
 *   = 0 : failure reason TIMEOUT
 *   > 0 : success
 */
int qmi_wda_set_qmap_settings(struct rmnet_context *context)
{
	int status = 0, request_size = 0;
	char *buffer = NULL, *req_offset = NULL, *read_offset = NULL;
	u16 transaction_id = 0;
	struct qmi_wda_set_qmap_settings_req *msg = NULL;
	struct qmi_wda_in_band_flow_control *flow_control;
	struct qmi_svc_response_header *response = NULL;
	
	flow_control = NULL;

	pr_debug(NET_DEVICE_NAME " qmi wda set qmap settings\n");
	
	request_size = sizeof(struct qmi_wda_set_qmap_settings_req);
#ifdef QMAP_IN_BAND_FLOW_CONTROL
	request_size += sizeof(struct qmi_wda_in_band_flow_control);
#endif

	if (request_size == sizeof(struct qmi_wda_set_qmap_settings_req)) {
		pr_err(NET_DEVICE_NAME " there is no qmap settings data\n");
		return 0;
	}
	
	transaction_id = (u16)qmi_svc_get_txid(&context->qmi_wda_txid);
	
	// allocate memory for send + response message buffer
	buffer = kmalloc(request_size + RMNET_RESPONSE_BUFFER_LEN, GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, request_size);

	// initialize send message
	msg = (struct qmi_wda_set_qmap_settings_req *)buffer;
	msg->message_id = QMI_WDA_SET_QMAP_SETTINGS;
	msg->length = request_size - sizeof(struct qmi_wda_set_qmap_settings_req);
	req_offset = buffer + sizeof(struct qmi_wda_set_qmap_settings_req);
#ifdef QMAP_IN_BAND_FLOW_CONTROL
	flow_control = (struct qmi_wda_in_band_flow_control *)req_offset;
	flow_control->type = 0x10;
	flow_control->length = 0x1;
	flow_control->flow_control = QMAP_IN_BAND_FLOW_CONTROL;
	req_offset += sizeof(struct qmi_wda_in_band_flow_control);
#endif
	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xc + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_WDA;
	msg->msg_header.client_id = context->qmi_wda_client_id;

	read_offset = buffer + request_size;

	// send message and wait response
	status = qmi_svc_send_and_wait(context, 
					RMNET_QMI_VERSION_WDA,
					msg->msg_header.client_id,
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1,
					msg->sdu_header.transaction_id,
					read_offset,
					RMNET_RESPONSE_BUFFER_LEN);

	// send failure
	if (status <= 0) {
		pr_err(NET_DEVICE_NAME " qmi wda set qmap settings error\n");
		goto exit;
	}

	// get response message buffer
	response = (struct qmi_svc_response_header *)read_offset;

	// qmi result error
	if (response->qmi_result != 0) {
		pr_err(NET_DEVICE_NAME " qmi wda set qmap settings result error\n");
		status = -1;
		goto exit;
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}

/*
 * QMI_WDS_BIND_MUX_DATA_PORT
 *
 * Parameters
 *   context : rmnet context
 *   transaction_id : transaction id
 *   client_id : client id
 *   bind_mux_id : mux id for binding
 *
 * Return value
 *   < 0 : failure reason ERROR
 *   = 0 : failure reason TIMEOUT
 *   > 0 : success
 */
int qmi_wds_bind_mux_data_port(struct rmnet_context *context, u16 transaction_id, u8 client_id, u8 bind_mux_id)
{
	int status = 0, request_size = 0;
	char *buffer = NULL, *req_offset = NULL, *read_offset = NULL;
	struct qmi_wds_mux_data_port_req *msg = NULL;
	struct qmi_wds_peripheral_end_point_id *ep_id;
	struct qmi_wds_mux_id *mux_id;
	struct qmi_wds_client_type *client_type;
	struct qmi_svc_response_header *response = NULL;
	
	ep_id = NULL;
	mux_id = NULL;
	client_type = NULL;

	pr_debug(NET_DEVICE_NAME " qmi wds bind mux data port\n");
	
	request_size = sizeof(struct qmi_wds_mux_data_port_req);
#ifdef PERIPHERAL_EP_ID_EP_TYPE
#ifdef PERIPHERAL_EP_ID_IFACE_ID
	request_size += sizeof(struct qmi_wds_peripheral_end_point_id);
#endif
#endif
#ifdef QMAP_MUX_ID
	request_size += sizeof(struct qmi_wds_mux_id);
#endif
#ifdef QMAP_CLIENT_TYPE
	request_size += sizeof(struct qmi_wds_client_type);
#endif

	if (request_size == sizeof(struct qmi_wds_mux_data_port_req)) {
		pr_err(NET_DEVICE_NAME " there is no bind mux port data\n");
		return 0;
	}

	// allocate memory for send + response message buffer
	buffer = kmalloc(request_size + RMNET_RESPONSE_BUFFER_LEN, GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, request_size);

	// initialize send message
	msg = (struct qmi_wds_mux_data_port_req *)buffer;
	msg->message_id = QMI_WDS_BIND_MUX_DATA_PORT;
	msg->length = request_size - sizeof(struct qmi_wds_mux_data_port_req);
	req_offset = buffer + sizeof(struct qmi_wds_mux_data_port_req);
#ifdef PERIPHERAL_EP_ID_EP_TYPE
#ifdef PERIPHERAL_EP_ID_IFACE_ID
	ep_id = (struct qmi_wds_peripheral_end_point_id *)req_offset;
	ep_id->type = 0x10;
	ep_id->length = 0x8;
	ep_id->ep_type = PERIPHERAL_EP_ID_EP_TYPE;
	ep_id->iface_id = PERIPHERAL_EP_ID_IFACE_ID;
	req_offset += sizeof(struct qmi_wds_peripheral_end_point_id);
#endif
#endif
#ifdef QMAP_MUX_ID
	mux_id = (struct qmi_wds_mux_id *)req_offset;
	mux_id->type = 0x11;
	mux_id->length = 0x1;
	mux_id->mux_id = bind_mux_id;
	req_offset += sizeof(struct qmi_wds_mux_id);
#endif
#ifdef QMAP_CLIENT_TYPE
	client_type = (struct qmi_wds_client_type *)req_offset;
	client_type->type = 0x13;
	client_type->length = 0x4;
	client_type->client_type = QMAP_CLIENT_TYPE;
	req_offset += sizeof(struct qmi_wds_client_type);
#endif
	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xc + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_WDS;
	msg->msg_header.client_id = client_id;

	read_offset = buffer + request_size;

	// send message and wait response
	status = qmi_svc_send_and_wait(context, 
					RMNET_QMI_VERSION_WDS,
					msg->msg_header.client_id,
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1,
					msg->sdu_header.transaction_id,
					read_offset,
					RMNET_RESPONSE_BUFFER_LEN);

	// send failure
	if (status <= 0) {
		pr_err(NET_DEVICE_NAME " qmi wds bind mux data port error, [%d, %d]\n", context->real_dev_idx, context->mux_dev_idx);
		goto exit;
	}

	// get response message buffer
	response = (struct qmi_svc_response_header *)read_offset;

	// qmi result error
	if (response->qmi_result != 0) {
		pr_err(NET_DEVICE_NAME " qmi wds bind mux data port result error [%d, %d]\n", context->real_dev_idx, context->mux_dev_idx);
		status = -1;
		goto exit;
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}

/*
 * QMI_WDS_SET_CLIENT_IP_FAMILY
 *
 * Parameters
 *   context : rmnet context
 *   transaction_id : transaction id
 *   client_id : client id
 *   ip_pref : ip family preference
 *
 * Return value
 *   < 0 : failure reason ERROR
 *   = 0 : failure reason TIMEOUT
 *   > 0 : success
 */
int qmi_wds_set_client_ip_family(struct rmnet_context *context, u16 transaction_id, u8 client_id, u8 ip_pref)
{
	int status = 0;

	char *buffer = NULL, *read_offset = NULL;
	struct qmi_wds_set_client_ip_family_pref_req *msg = NULL;

	struct qmi_svc_response_header *response = NULL;

	pr_debug(NET_DEVICE_NAME " qmi wds set client ip family pref\n");

	// allocate memory for send + response message buffer
	buffer = kmalloc(sizeof(struct qmi_wds_set_client_ip_family_pref_req) + RMNET_RESPONSE_BUFFER_LEN, GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, sizeof(struct qmi_wds_set_client_ip_family_pref_req));

	// initialize send message
	msg = (struct qmi_wds_set_client_ip_family_pref_req *)buffer;
	msg->message_id = QMI_WDS_SET_CLIENT_IP_FAMILY_PREF;
	msg->length = sizeof(struct qmi_wds_ip_family_preference);

	msg->ip_preference.type = 0x1;
	msg->ip_preference.length = 0x1;
	msg->ip_preference.ip_preference = ip_pref;

	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xc + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_WDS;
	msg->msg_header.client_id = client_id;

	read_offset = buffer + sizeof(struct qmi_wds_set_client_ip_family_pref_req);

	// send message and wait response
	status = qmi_svc_send_and_wait(context, 
					RMNET_QMI_VERSION_WDS,
					msg->msg_header.client_id,
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1,
					msg->sdu_header.transaction_id,
					read_offset,
					RMNET_RESPONSE_BUFFER_LEN);

	// send failure
	if (status <= 0) {
		pr_err(NET_DEVICE_NAME " qmi wds set client ip family pref error\n");
		goto exit;
	}

	// get response message buffer
	response = (struct qmi_svc_response_header *)read_offset;

	// qmi result error
	if (response->qmi_result != 0) {
		pr_err(NET_DEVICE_NAME " qmi wds set client ip family pref result error %d\n", response->qmi_error);
		status = -1;
		goto exit;
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}


/*
 * QMI_WDS_BIND_SUBSCRIPTION
 *
 * Parameters
 *   context : rmnet context
 *   transaction_id : transaction id
 *   client_id : client id
 *   subscription : subscription number for binding
 *
 * Return value
 *   < 0 : failure reason ERROR
 *   = 0 : failure reason TIMEOUT
 *   > 0 : success
 */
int qmi_wds_bind_subscription(struct rmnet_context *context, u16 transaction_id, u8 client_id, u32 subscription)
{
	int status = 0;

	char *buffer = NULL, *read_offset = NULL;
	struct qmi_wds_bind_subscription_req *msg = NULL;

	struct qmi_svc_response_header *response = NULL;

	pr_debug(NET_DEVICE_NAME " qmi_wds_bind_subscription_req start\n");

	// allocate memory for send + response message buffer
	buffer = kmalloc(sizeof(struct qmi_wds_bind_subscription_req) + RMNET_RESPONSE_BUFFER_LEN, GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, sizeof(struct qmi_wds_bind_subscription_req));

	// initialize send message
	msg = (struct qmi_wds_bind_subscription_req *)buffer;
	msg->message_id = QMI_WDS_BIND_SUBSCRIPTION;
	msg->length = sizeof(struct qmi_wds_bind_subscription_tlv);

	msg->subscription.type = 0x1;
	msg->subscription.length = 0x4;
	msg->subscription.subscription= subscription;

	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xc + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_WDS;
	msg->msg_header.client_id = client_id;

	read_offset = buffer + sizeof(struct qmi_wds_bind_subscription_req);

	// send message and wait response
	status = qmi_svc_send_and_wait(context, 
					RMNET_QMI_VERSION_WDS,
					msg->msg_header.client_id,
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1,
					msg->sdu_header.transaction_id,
					read_offset,
					RMNET_RESPONSE_BUFFER_LEN);

	// send failure
	if (status <= 0) {
		//dev_err(&context->intf->dev, "qmi_wds_bind_subscription_req error\n");
		printk(KERN_NOTICE "[in func] qmi_wds_bind_subscription_req [%d, %d] [status : %d] <client_id : %d> \n", context->real_dev_idx, context->mux_dev_idx,status, client_id);
		goto exit;
	}

	// get response message buffer
	response = (struct qmi_svc_response_header *)read_offset;

	// qmi result error
	if (response->qmi_result != 0) {
		//dev_err(&context->intf->dev, "qmi_wds_bind_subscription_req error result error %d\n", response->qmi_error);
		printk(KERN_NOTICE "[in func] qmi_wds_bind_subscription_req qmi_result error [%d, %d] [qmi_result : %d] <client_id : %d>\n", context->real_dev_idx, context->mux_dev_idx, response->qmi_result, client_id);
		status = -1;
		goto exit;
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}


int qmi_wds_start_network_interface(struct rmnet_context *context, u16 transaction_id, u8 client_id, u8 technology_preference, u8 profile_index)
{
	int status = 0;

	char *buffer = NULL, *read_offset = NULL;
	struct qmi_wds_start_network_interface_req *msg = NULL;

	struct qmi_svc_response_header *response = NULL;

	pr_debug(NET_DEVICE_NAME " qmi_wds_start_network_interface start\n");

	// allocate memory for send + response message buffer
	buffer = kmalloc(sizeof(struct qmi_wds_start_network_interface_req) + RMNET_RESPONSE_BUFFER_LEN, GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, sizeof(struct qmi_wds_start_network_interface_req));

	// initialize send message
	msg = (struct qmi_wds_start_network_interface_req *)buffer;
	msg->message_id = QMI_WDS_START_NETWORK_INTERFACE;
	msg->length = sizeof(struct qmi_wds_start_network_interface_tlv);

	msg->tlvs.type1= 0x30;
	msg->tlvs.length1= 0x1;
	msg->tlvs.technology_preference= technology_preference;

	msg->tlvs.type2= 0x31;
	msg->tlvs.length2= 0x1;
	msg->tlvs.profile_index= profile_index;

	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xc + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_WDS;
	msg->msg_header.client_id = client_id;

	read_offset = buffer + sizeof(struct qmi_wds_start_network_interface_req);

	// send message and wait response
	status = qmi_svc_send_and_wait(context, 
					RMNET_QMI_VERSION_WDS,
					msg->msg_header.client_id,
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1,
					msg->sdu_header.transaction_id,
					read_offset,
					RMNET_RESPONSE_BUFFER_LEN);

	// send failure
	if (status <= 0) {
		//dev_err(&context->intf->dev, "qmi_wds_start_network_interface error %d\n", status);
		printk(KERN_NOTICE "[in_func] qmi_wds_start_network_interface [status %d] [%d %d] <client id : %d>\n", status, context->real_dev_idx, context->mux_dev_idx, client_id);
		goto exit;
	}

	// get response message buffer
	response = (struct qmi_svc_response_header *)read_offset;

	// qmi result error
	if (response->qmi_result != 0) {
		//dev_err(&context->intf->dev, "qmi_wds_start_network_interface result error %d\n", response->qmi_error);
		printk(KERN_NOTICE "[in_func] qmi_wds_start_network_interface qmi fail. [qmi_result : %d] [%d %d] <client id : %d>\n", response->qmi_result, context->real_dev_idx, context->mux_dev_idx, client_id);
		status = -1;
		goto exit;
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}

/*
 * QMI_WDS_INDICATION_REGISTER
 *
 * Parameters
 *   context : rmnet context
 *   transaction_id : transaction id
 *   client_id : client id
 *
 * Return value
 *   < 0 : failure reason ERROR
 *   = 0 : failure reason TIMEOUT
 *   > 0 : success
 */
int qmi_wds_indication_register(struct rmnet_context *context, u16 transaction_id, u8 client_id)
{
	int status = 0;

	char *buffer = NULL, *read_offset = NULL;
	struct qmi_wds_indication_register_req *msg = NULL;

	struct qmi_svc_response_header *response = NULL;

	pr_debug(NET_DEVICE_NAME " qmi indication register\n");

	// allocate memory for send + response message buffer
	buffer = kmalloc(sizeof(struct qmi_wds_indication_register_req) + RMNET_RESPONSE_BUFFER_LEN, GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, sizeof(struct qmi_wds_indication_register_req));

	// initialize send message
	msg = (struct qmi_wds_indication_register_req *)buffer;
	msg->message_id = QMI_WDS_INDICATION_REGISTER;
	msg->length = sizeof(struct qmi_wds_extended_ip_configuration_change);

	msg->report_extended_ip_config_change.type = 0x12;
	msg->report_extended_ip_config_change.length = 0x1;
	msg->report_extended_ip_config_change.report_extended_ip_config_change = 0x1;

	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xc + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_WDS;
	msg->msg_header.client_id = client_id;

	read_offset = buffer + sizeof(struct qmi_wds_indication_register_req);

	// send message and wait response
	status = qmi_svc_send_and_wait(context, 
					RMNET_QMI_VERSION_WDS,
					msg->msg_header.client_id,
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1,
					msg->sdu_header.transaction_id,
					read_offset,
					RMNET_RESPONSE_BUFFER_LEN);

	// send failure
	if (status <= 0) {
		pr_err(NET_DEVICE_NAME " qmi wds indication register error\n");
		goto exit;
	}

	// get response message buffer
	response = (struct qmi_svc_response_header *)read_offset;

	// qmi result error
	if (response->qmi_result != 0) {
		pr_err(NET_DEVICE_NAME " qmi wds indication register result error\n");
		status = -1;
		goto exit;
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}

/*
 * QMI_WDS_SET_EVENT_REPORT
 *
 * Parameters
 *   context : rmnet context
 *   transaction_id : transaction id
 *   client_id : client id
 *
 * Return value
 *   < 0 : error
 *   = 0 : success
 */
int qmi_wds_set_event_report(struct rmnet_context *context, u16 transaction_id, u8 client_id)
{
	int status = 0;

	char *buffer = NULL;
	struct qmi_wds_set_event_report_req *msg = NULL;

	pr_debug(NET_DEVICE_NAME " qmi wds set event report\n");

	// allocate memory for send message buffer
	buffer = kmalloc(sizeof(struct qmi_wds_set_event_report_req), GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, sizeof(struct qmi_wds_set_event_report_req));

	// initialize send message
	msg = (struct qmi_wds_set_event_report_req *)buffer;
	msg->message_id = QMI_WDS_SET_EVENT_REPORT;
	msg->length = sizeof(struct qmi_wds_channel_rate_indicator);

	msg->channel_rate.type = 0x10;
	msg->channel_rate.length = 0x01;
	msg->channel_rate.report_indicator = 0x1;
	
	msg->transfer_stat.type = 0x11;
	msg->transfer_stat.length = 0x05;
	msg->transfer_stat.stats_period = 0x0;
	msg->transfer_stat.stats_mask = 0x0;
	
	msg->bearer_tech.type = 0x12;
	msg->bearer_tech.length = 0x01;
	msg->bearer_tech.report_bearer = 0x1;
	
	msg->dormancy_stat.type = 0x13;
	msg->dormancy_stat.length = 0x01;
	msg->dormancy_stat.report_dormancy = 0x1;

	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xc + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_WDS;
	msg->msg_header.client_id = client_id;

	// send message and no wait response
	status = qmi_svc_send_and_forget(context, 
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1);

	// send failure
	if (status < 0) {
		pr_err(NET_DEVICE_NAME " qmi wds set event report error\n");
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}

/*
 * QMI_WDS_GET_RUNTIME_SETTINGS
 *
 * Parameters
 *   context : rmnet context
 *   transaction_id : transaction id
 *   client_id : client id
 *
 * Return value
 *   < 0 : failure reason ERROR
 *   = 0 : failure reason TIMEOUT
 *   > 0 : success
 */
int qmi_wds_get_runtime_settings(struct rmnet_context *context, u16 transaction_id, u8 client_id)
{
	int status = 0;

	char *buffer = NULL, *read_offset = NULL;
	struct qmi_wds_get_runtime_settings_req *msg = NULL;

	struct qmi_svc_response_header *response = NULL;
	struct qmi_wds_ipv4_address_pref *ipv4_address = NULL;
	struct qmi_wds_ipv6_address_pref *ipv6_address = NULL;

	pr_debug(NET_DEVICE_NAME " qmi wds get runtime settings\n");

	// allocate memory for send + response message buffer
	buffer = kmalloc(sizeof(struct qmi_wds_get_runtime_settings_req) + RMNET_RESPONSE_BUFFER_LEN, GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, sizeof(struct qmi_wds_get_runtime_settings_req));

	// initialize send message
	msg = (struct qmi_wds_get_runtime_settings_req *)buffer;
	msg->message_id = QMI_WDS_GET_RUNTIME_SETTINGS;
	msg->length = sizeof(struct qmi_wds_runtime_requested);

	msg->requested_settings.type = 0x10;
	msg->requested_settings.length = 0x4;
	msg->requested_settings.requested_settings = 0x2310;

	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xc + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_WDS;
	msg->msg_header.client_id = client_id;

	read_offset = buffer + sizeof(struct qmi_wds_indication_register_req);

	// send message and wait response
	status = qmi_svc_send_and_wait(context, 
					RMNET_QMI_VERSION_WDS,
					msg->msg_header.client_id,
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1,
					msg->sdu_header.transaction_id,
					read_offset,
					RMNET_RESPONSE_BUFFER_LEN);

	// send failure
	if (status <= 0) {
		pr_err(NET_DEVICE_NAME " qmi wds get runtime settings error\n");
		goto exit;
	}

	// get response message buffer
	response = (struct qmi_svc_response_header *)read_offset;

	// qmi result error
	if (response->qmi_result != 0) {
		pr_err(NET_DEVICE_NAME " qmi wds get runtime settings result error\n");
		status = -1;
		goto exit;
	}
	
	ipv4_address = (struct qmi_wds_ipv4_address_pref *)qmi_svc_find_tlv(&response->tlv_type, 0x1E, response->length);
	if (ipv4_address) {
		pr_debug(NET_DEVICE_NAME " IPv4 address : %d.%d.%d.%d\n", 
			(ipv4_address->ipv4_address >> 24) & 0xFF,
			(ipv4_address->ipv4_address >> 16) & 0xFF,
			(ipv4_address->ipv4_address >> 8) & 0xFF,
			ipv4_address->ipv4_address & 0xFF);
		context->ipv4_address = htonl(ipv4_address->ipv4_address);
	}
	
	ipv6_address = (struct qmi_wds_ipv6_address_pref *)qmi_svc_find_tlv(&response->tlv_type, 0x25, response->length);
	if (ipv6_address) {
		pr_debug(NET_DEVICE_NAME " IPv6 address : %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x/%d\n", 
			ipv6_address->ipv6_address[0], ipv6_address->ipv6_address[1],
			ipv6_address->ipv6_address[2], ipv6_address->ipv6_address[3],
			ipv6_address->ipv6_address[4], ipv6_address->ipv6_address[5],
			ipv6_address->ipv6_address[6], ipv6_address->ipv6_address[7],
			ipv6_address->ipv6_address[8], ipv6_address->ipv6_address[9],
			ipv6_address->ipv6_address[10], ipv6_address->ipv6_address[11],
			ipv6_address->ipv6_address[12], ipv6_address->ipv6_address[13],
			ipv6_address->ipv6_address[14], ipv6_address->ipv6_address[15],
			ipv6_address->ipv6_prefix_len);
		memcpy(context->ipv6_address, ipv6_address->ipv6_address, 16);
		context->ipv6_prefix_len = ipv6_address->ipv6_prefix_len;
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}

int qmi_wds_get_runtime_settings_async(struct rmnet_context *context, u16 transaction_id, u8 client_id, u32 req_settings)
{
	int status = 0;

	char *buffer = NULL, *read_offset = NULL;
	struct qmi_wds_get_runtime_settings_req *msg = NULL;

	pr_debug(NET_DEVICE_NAME " qmi wds get runtime settings ex\n");

	// allocate memory for send + response message buffer
	buffer = kmalloc(sizeof(struct qmi_wds_get_runtime_settings_req), GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, sizeof(struct qmi_wds_get_runtime_settings_req));

	// initialize send message
	msg = (struct qmi_wds_get_runtime_settings_req *)buffer;
	msg->message_id = QMI_WDS_GET_RUNTIME_SETTINGS;
	msg->length = sizeof(struct qmi_wds_runtime_requested);

	msg->requested_settings.type = 0x10;
	msg->requested_settings.length = 0x4;
	msg->requested_settings.requested_settings = req_settings;

	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xc + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_WDS;
	msg->msg_header.client_id = client_id;

	read_offset = buffer + sizeof(struct qmi_wds_indication_register_req);

		// send message and no wait response
	status = qmi_svc_send_and_forget(context, 
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1);

	// send failure
	if (status < 0) {
		pr_err(NET_DEVICE_NAME " qmi wds get current channel rate error\n");
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}

int qmi_wds_get_runtime_settings_resp(struct rmnet_context *context, void *msg)
{
	int status = 0;
	
	struct qmi_svc_response_header *header = NULL;
	struct qmi_wds_mtu *mtu = NULL;
	unsigned char *tlvs = NULL;
	
	pr_debug(NET_DEVICE_NAME " qmi wds get runtime settings response\n");
	
	// find packet service status tlv
	header = (struct qmi_svc_response_header *)msg;
	tlvs = (unsigned char *)&header->tlv_type;
	mtu = (struct qmi_wds_mtu *)qmi_svc_find_tlv(tlvs, 0x29, header->length);

	if (mtu) {
		pr_debug(NET_DEVICE_NAME " update mtu : %d\n", mtu->mtu);
		usbnet_change_mtu(context->netdev, mtu->mtu);
	}
	
	return status;
}

/*
 * QMI_WDS_GET_CURRENT_CHANNEL_RATE_REQUEST
 *
 * Parameters
 *   context : rmnet context
 *   transaction_id : transaction id
 *   client_id : client id
 *
 * Return value
 *   < 0 : failure reason ERROR
 *   = 0 : failure reason TIMEOUT
 *   > 0 : success
 */
int qmi_wds_get_current_channel_rate(struct rmnet_context *context, u16 transaction_id, u8 client_id)
{
	int status = 0;

	char *buffer = NULL, *read_offset = NULL;
	struct qmi_wds_get_current_channel_rate_req *msg = NULL;

	pr_debug(NET_DEVICE_NAME " qmi wds get current channel rate\n");

	// allocate memory for send message buffer
	buffer = kmalloc(sizeof(struct qmi_wds_get_current_channel_rate_req), GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, sizeof(struct qmi_wds_get_current_channel_rate_req));

	// initialize send message
	msg = (struct qmi_wds_get_current_channel_rate_req *)buffer;
	msg->message_id = QMI_WDS_GET_CURRENT_CHANNEL_RATE;
	msg->length = 0;

	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xc + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_WDS;
	msg->msg_header.client_id = client_id;

	read_offset = buffer + sizeof(struct qmi_wds_indication_register_req);

	// send message and no wait response
	status = qmi_svc_send_and_forget(context, 
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1);

	// send failure
	if (status < 0) {
		pr_err(NET_DEVICE_NAME " qmi wds get current channel rate error\n");
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}

/*
 * QMI_WDS_GET_CURRENT_CHANNEL_RATE_RESPONSE
 *
 * Parameters
 *   context : rmnet context
 *   msg : response message
 *
 * Return value
 *   < 0 : error
 *   = 0 : success
 */
int qmi_wds_get_current_channel_rate_resp(struct rmnet_context *context, void *msg)
{
	int status = 0;

	struct qmi_svc_response_header *header = NULL;
	struct qmi_wds_current_channel_rate *rate = NULL;

	pr_debug(NET_DEVICE_NAME " qmi wds get current channel rate response\n");

	// find packet channel rate tlv
	header = (struct qmi_svc_response_header *)msg;
	rate = (struct qmi_wds_current_channel_rate *)qmi_svc_find_tlv((unsigned char *)&header->tlv_type, 0x01, header->length);

	// no channel rate tlv, return error
	if (rate == NULL) {
		status = -1;
		goto exit;
	}

	pr_info(NET_DEVICE_NAME
			" current channel rate(cid 0x%x) : %u bps up, %u bps down\n",
			header->msg_header.client_id,
			__le32_to_cpu(rate->current_tx_rate),
			__le32_to_cpu(rate->current_rx_rate));

exit :

	return status;
}

/*
 * QMI_WDS_EVENT_REPORT_IND
 *
 * Parameters
 *   context : rmnet context
 *   msg : response message
 *
 * Return value
 *   < 0 : error
 *   = 0 : success
 */
int qmi_wds_event_report_ind(struct rmnet_context *context, void *msg)
{
	int status = 0;
	u8 client_id = 0;
	struct qmi_svc_response_header *header = NULL;

	pr_debug(NET_DEVICE_NAME " qmi wds event report ind\n");
	
	header = (struct qmi_svc_response_header *)msg;
	client_id = header->msg_header.client_id;
	
	status = qmi_wds_get_current_channel_rate(
				context, 
				(u16)qmi_svc_get_txid(&context->qmi_wds_txid), 
				client_id);

	return status;
}

/*
 * QMI_WDS_GET_PKT_SRVC_STATUS
 *
 * Parameters
 *   context : rmnet context
 *   ip_family : ip family preference
 *
 * Return value
 *   < 0 : error
 *   = 0 : success
 */
int qmi_wds_get_pkt_srvc_status(struct rmnet_context *context, u8 ip_family)
{
	int status = 0;

	char *buffer = NULL;
	u16 transaction_id = 0;
	u8 client_id;
	struct qmi_wds_get_pkt_srvc_status_req *msg = NULL;

	pr_debug(NET_DEVICE_NAME " qmi wds get pkt srvc status\n");

	transaction_id = (u16)qmi_svc_get_txid(&context->qmi_wds_txid);

	// allocate memory for send message buffer
	buffer = kmalloc(sizeof(struct qmi_wds_get_pkt_srvc_status_req), GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}
	
	if (ip_family == QMI_WDS_IP_FAMILY_IPV6 && 
		context->qmi_wds_client_ex > 0) {
		client_id = context->qmi_wds_client_ex;
	} else {
		client_id = context->qmi_wds_client_id;
	}

	memset(buffer, 0, sizeof(struct qmi_wds_get_pkt_srvc_status_req));

	// initialize send message
	msg = (struct qmi_wds_get_pkt_srvc_status_req *)buffer;
	msg->message_id = QMI_WDS_GET_PKT_SRVC_STATUS;
	msg->length = 0;

	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xc + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_WDS;
	msg->msg_header.client_id = client_id;

	// send message and no wait response
	status = qmi_svc_send_and_forget(context, 
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1);

	// send failure
	if (status < 0) {
		pr_err(NET_DEVICE_NAME " qmi wds get pkt srvc status error\n");
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}

/*
 * QMI_WDS_GET_PKT_SRVC_STATUS_RESP
 *
 * Parameters
 *   context : rmnet context
 *   msg : response message
 *
 * Return value
 *   < 0 : error
 *   = 0 : success
 */
int qmi_wds_get_pkt_srvc_status_resp(struct rmnet_context *context, void *msg)
{
	int status = 0;
	
	struct qmi_svc_response_header *header = NULL;
	struct qmi_wds_connection_status *conn_status = NULL;
	unsigned char *tlvs = NULL;
	u8 ip_family;
	
	pr_debug(NET_DEVICE_NAME " qmi wds get pkt srvc status response\n");
	
	// find packet service status tlv
	header = (struct qmi_svc_response_header *)msg;
	tlvs = (unsigned char *)&header->tlv_type;
	conn_status = (struct qmi_wds_connection_status *)qmi_svc_find_tlv(tlvs, 0x01, header->length);

	// no connection status tlv, return error
	if (conn_status == NULL) {
		pr_debug(NET_DEVICE_NAME " there is no connection status\n");
		status = -1;
		goto exit;
	}
	
	if (header->msg_header.client_id == context->qmi_wds_client_ex) {
		ip_family = QMI_WDS_IP_FAMILY_IPV6;
	} else {
		ip_family = QMI_WDS_IP_FAMILY_IPV4;
	}
	
	pr_debug(NET_DEVICE_NAME " wds connection status is %d(IPv%d)\n", conn_status->connection_status, ip_family);
	
	switch (conn_status->connection_status) {
	case 0x01 : // disconnected

		memset(&context->netdev->stats, 0, sizeof(struct net_device_stats));
		netif_carrier_off(context->netdev);

		if (ip_family == QMI_WDS_IP_FAMILY_IPV6) {
			context->connect_v6 = CONNECTION_DISCONNECTED;
			memset(context->ipv6_address, 0, 17);
			context->ipv6_prefix_len = 0;
		} else {
			context->connect_v4 = CONNECTION_DISCONNECTED;
			context->ipv4_address = 0;
		}

		break;

	case 0x02 : // connected
		netif_carrier_off(context->netdev);

		if (ip_family == QMI_WDS_IP_FAMILY_IPV6) {
			context->connect_v6 = CONNECTION_CONNECTED;
		} else {
			context->connect_v4 = CONNECTION_CONNECTED;
		}

		schedule_work(&context->rmnet_carrier_on_work);
		break;
	}
	
exit :

	return status;
}

/*
 * QMI_WDS_PKT_SRVC_STATUS_IND
 *
 * Parameters
 *   context : rmnet context
 *   msg : response message
 *
 * Return value
 *   < 0 : error
 *   = 0 : success
 */
int qmi_wds_pkt_srvc_status_ind(struct rmnet_context *context, void *msg)
{
	int status = 0;

	struct qmi_svc_response_header *header = NULL;
	struct qmi_wds_packet_service_status *pkt_srvc_status = NULL;
	struct qmi_wds_call_end_reason *reason = NULL;
	struct qmi_wds_verbose_call_end_reason *verbose = NULL;
	struct qmi_wds_ip_family *family = NULL;
	unsigned char *tlvs = NULL;
	u8 ip_family;
	
	pr_debug(NET_DEVICE_NAME " qmi wds pkt srvc status\n");
	
	// find packet service status tlv
	header = (struct qmi_svc_response_header *)msg;
	tlvs = (unsigned char *)&header->tlv_type;
	pkt_srvc_status = (struct qmi_wds_packet_service_status *)qmi_svc_find_tlv(tlvs, 0x01, header->length);
	reason = (struct qmi_wds_call_end_reason *)qmi_svc_find_tlv(tlvs, 0x10, header->length);
	verbose = (struct qmi_wds_verbose_call_end_reason *)qmi_svc_find_tlv(tlvs, 0x11, header->length);
	family = (struct qmi_wds_ip_family *)qmi_svc_find_tlv(tlvs, 0x12, header->length);

	// no packet service status tlv, return error
	if (pkt_srvc_status == NULL) {
		pr_debug(NET_DEVICE_NAME " there is no packet service status\n");
		status = -1;
		goto exit;
	}
	
	if (family) {
		ip_family = family->ip_family;
	} else {
		if (header->msg_header.client_id == context->qmi_wds_client_ex) {
			ip_family = QMI_WDS_IP_FAMILY_IPV6;
		} else {
			ip_family = QMI_WDS_IP_FAMILY_IPV4;
		}
	}
	
	pr_debug(NET_DEVICE_NAME " wds packet service status is %d(IPv%d)\n", pkt_srvc_status->connection_status, ip_family);

	// proccess status
	switch (pkt_srvc_status->connection_status) {
	case 0x01 : // disconnected

		memset(&context->netdev->stats, 0, sizeof(struct net_device_stats));
		netif_carrier_off(context->netdev);

		if (ip_family == QMI_WDS_IP_FAMILY_IPV6) {

			printk(KERN_NOTICE "qmi wds pkt srvc status ind discon - ipv6 [%d %d]\n", context->real_dev_idx, context->mux_dev_idx);
			
			context->connect_v6 = CONNECTION_DISCONNECTED;
			memset(context->ipv6_address, 0, 17);
			context->ipv6_prefix_len = 0;
			qmi_wds_get_pkt_srvc_status(context, QMI_WDS_IP_FAMILY_IPV6);
		} else {

			printk(KERN_NOTICE "qmi wds pkt srvc status ind discon - ipv4 [%d %d]\n", context->real_dev_idx, context->mux_dev_idx);
		
			context->connect_v4 = CONNECTION_DISCONNECTED;
			context->ipv4_address = 0;
			qmi_wds_get_pkt_srvc_status(context, QMI_WDS_IP_FAMILY_IPV4);
		}

		break;

	case 0x02 : // connected
		
		netif_carrier_off(context->netdev);

		if (ip_family == QMI_WDS_IP_FAMILY_IPV6) {

			printk(KERN_NOTICE "qmi wds pkt srvc status ind conn - ipv6. [%d %d]\n", context->real_dev_idx, context->mux_dev_idx);
			
			context->connect_v6 = CONNECTION_CONNECTED;
		} else {

			printk(KERN_NOTICE "qmi wds pkt srvc status ind conn - ipv4 [%d %d]\n", context->real_dev_idx, context->mux_dev_idx);
		
			context->connect_v4 = CONNECTION_CONNECTED;
		}

		schedule_work(&context->rmnet_carrier_on_work);
		break;
	}

exit :

	return status;
}

/*
 * QMI_WDS_EXTENDED_IP_CONFIG_IND
 *
 * Parameters
 *   context : rmnet context
 *   msg : response message
 *
 * Return value
 *   < 0 : error
 *   = 0 : success
 */
int qmi_wds_extended_ip_config_ind(struct rmnet_context *context, void *msg)
{
	int status = 0;
	u8 client_id = 0;
	unsigned char *tlvs = NULL;
	struct qmi_wds_changed_ip_config *config = NULL;
	struct qmi_svc_response_header *header = NULL;

	pr_debug(NET_DEVICE_NAME " qmi wds extended ip config ind\n");
	
	header = (struct qmi_svc_response_header *)msg;
	tlvs = (unsigned char *)&header->tlv_type;
	config = (struct qmi_wds_changed_ip_config *)qmi_svc_find_tlv(tlvs, 0x10, header->length);
	client_id = header->msg_header.client_id;
	
	if (config == NULL) {
		pr_debug(NET_DEVICE_NAME " there is no change ip configuration\n");
		goto exit;
	}
	
	status = qmi_wds_get_runtime_settings_async(
				context,
				(u16)qmi_svc_get_txid(&context->qmi_wds_txid), 
				client_id,
				config->changed_ip_config);

exit :
	
	return status;
}

/*
 * process internal wds service response
 *
 * Parameters
 *   context : rmnet context
 *   msg : response message
 *
 * Return value
 *   < 0 : error
 *   = 0 : success
 */
int qmi_interal_wds_proc(struct rmnet_context *context, void *msg)
{
	int status = 0, is_response = 0;

	struct qmi_svc_response_header *header = NULL;

	// set response header
	header = (struct qmi_svc_response_header *)msg;

	pr_debug(NET_DEVICE_NAME " Message for internal wds service (message id=0x%x)\n", header->message_id);
	
	if (header->sdu_header.control_flags & 0x02) {
		is_response = 1;
	}

	// dispatch message handler
	switch (header->message_id) {
	case QMI_WDS_EVENT_REPORT_IND :
		if (!is_response) {
			qmi_wds_event_report_ind(context, msg);
		}
		break;

	case QMI_WDS_GET_PKT_SRVC_STATUS :
		if (!is_response) {
			qmi_wds_pkt_srvc_status_ind(context, msg);
		} else {
			qmi_wds_get_pkt_srvc_status_resp(context, msg);
		}
		break;

	case QMI_WDS_GET_CURRENT_CHANNEL_RATE :
		qmi_wds_get_current_channel_rate_resp(context, msg);
		break;

	case QMI_WDS_EXTENDED_IP_CONFIG_IND :
		qmi_wds_extended_ip_config_ind(context, msg);
		break;
		
	case QMI_WDS_GET_RUNTIME_SETTINGS :
		qmi_wds_get_runtime_settings_resp(context, msg);
		break;
	}

	return status;
}

/*
 * file read handler
 *
 * Parameters
 *   filp : file pointer
 *   buf : user buffer
 *   count : user buffer size
 *   f_pos : 
 *
 * Return value
 *   < 0 : error
 *   = 0 : 
 *   > 0 : read message size
 */
ssize_t qmi_svc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int result = 0;
	struct rmnet_context *context = NULL;
	struct qmi_svc_context *svc_context = NULL;

	struct list_entry *node = NULL;
	struct qmi_svc_data_list *data_list = NULL;
	u8 remove_pending = 0;

	DECLARE_WAIT_QUEUE_HEAD(read_wait_queue);
	
	if(filp == NULL)
		return -EINVAL;

	svc_context = filp->private_data;

	if(svc_context == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_read - service context is null - return\n");
		return -EINVAL;
	}
	
	context = svc_context->parent_context;

	if(context == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_read - interface context is null - return\n");
		return -EINVAL;		
	}
	
	pr_debug(NET_DEVICE_NAME " QMI service read (filp=0x%p context=0x%p length=%zd)\n", (void *)filp, (void *)svc_context, count);
	
	if (RMNET_STATUS_REMOVED & context->rmnet_status) {
		pr_err(NET_DEVICE_NAME " read error : device already removed\n");
		return -ENODEV;
	}
	
	mutex_lock(&context->cxt_lock);
	
	// if no received message, wait message.
	node = svc_context->data_queue.prev;
	if (node == &svc_context->data_queue) {
		svc_context->read_wait_queue = &read_wait_queue;
		svc_context->read_wait_active = 1;

		mutex_unlock(&context->cxt_lock);
		wait_event_interruptible(read_wait_queue, svc_context->read_wait_active == 0);
		mutex_lock(&context->cxt_lock);
	}

	pr_debug(NET_DEVICE_NAME " stop - read wait (%d) \n", svc_context->read_wait_active);

	svc_context->read_wait_active = 0;
	svc_context->read_wait_queue = NULL;

	if(get_remove_pending(svc_context->real_dev_idx, &remove_pending) < 0){
		mutex_unlock(&context->cxt_lock);
		return -ENODEV;
	}

	if(remove_pending == 1){
		mutex_unlock(&context->cxt_lock);
		return -ENODEV;
	}

	// received message process
	node = svc_context->data_queue.prev;
	if (node != &svc_context->data_queue) {
		// copy response message to user buffer
		remove_list(node);
		data_list = (struct qmi_svc_data_list *)container_of(node, struct qmi_svc_data_list, list);
		memcpy(buf, data_list->buffer, data_list->length);
		result = data_list->length;
	} else {
		// if no received message, return error
		pr_err(NET_DEVICE_NAME " there is no received message\n");
		result = svc_context->error_no;
	}

	mutex_unlock(&context->cxt_lock);	

	return result;
}

/*
 * file write handler
 *
 * Parameters
 *   filp : file pointer
 *   buf : user buffer
 *   count : user buffer size
 *   f_pos : 
 *
 * Return value
 *   < 0 : error
 *   = 0 : 
 *   > 0 : write message size
 */
ssize_t qmi_svc_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int result = 0;
	struct rmnet_context *context = NULL;
	struct qmi_svc_context *svc_context = NULL;

	struct qmi_qmux_msg_header *msg = NULL;
	u8 remove_pending = 0;
	
	if(filp == NULL)
		return -EINVAL;

	svc_context = filp->private_data;

	if(svc_context == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_write - service context is null - return\n");
		return -EINVAL;		
	}
	
	context = svc_context->parent_context;

	if(context == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_write - interface context is null - return\n");
		return -EINVAL;
	}

	if(get_remove_pending(svc_context->real_dev_idx, &remove_pending) < 0){
		pr_warn(NET_DEVICE_NAME " qmi_svc_write - remove pending check - return\n");
		return -ENODEV;
	}

	if(remove_pending == 1){
		pr_warn(NET_DEVICE_NAME " qmi_svc_write - remove pending condition - return\n");
		return -ENODEV;		
	}

	if(get_real_dev(svc_context->real_dev_idx) == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_write - device is null - return\n");
		return -ENODEV;
	}

	if(get_mux_dev(svc_context->real_dev_idx, svc_context->mux_dev_idx) == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_write - adapter is null - return\n");
		return -ENODEV;
	}
	
	pr_debug(NET_DEVICE_NAME " QMI service write (filp=0x%p context=0x%p length=%zd)\n", (void *)filp, (void *)svc_context, count);
	
	if (RMNET_STATUS_REMOVED & context->rmnet_status) {
		pr_err(NET_DEVICE_NAME " write error : device already removed\n");
		return -ENODEV;
	}

	// allocate message buffer : write message size from user + header length
	msg = kmalloc(count + sizeof(struct qmi_qmux_msg_header), GFP_KERNEL);

	if (msg == NULL) {
		pr_warn(NET_DEVICE_NAME " message buffer allocation failed\n");
		return -1;
	}

	// initialize header
	msg->if_type = 0x1;
	msg->length = count + sizeof(struct qmi_qmux_msg_header) - 1;
	msg->control_flags = 0x0;
	msg->service_type = svc_context->service_type;
	msg->client_id = svc_context->client_id;
	memcpy((void *)(msg+1), buf, count);

	// send message
	result = qmi_svc_send_and_forget(context, msg, msg->length + 1);

	pr_debug(NET_DEVICE_NAME " send message %d bytes\n", result);

	// if send message success, return original message length
	if (result > 0)
		result = count;

	return result;
}

/*
 * file i/o control handler
 *
 * Parameters
 *   filp : file pointer
 *   cmd : i/o control command
 *   arg : argument
 *
 * Return value
 *   < 0 : error
 *   = 0 : 
 *   > 0 : write message size
 */
long qmi_svc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int result = 0;
	struct rmnet_context *context = NULL;
	struct qmi_svc_context *svc_context = NULL;
	u8 real_dev_idx = 0;

	u8 service_type = 0;
	u8 remove_pending = 0;
	
	if(filp == NULL)
		return -EINVAL;

	svc_context = filp->private_data;

	if(svc_context == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_ioctl - service context is null - return\n");
		return -EINVAL;
	}

	if(get_real_dev(svc_context->real_dev_idx) == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_ioctl return - device is null\n");
		return -ENODEV;
	}

	if(get_mux_dev(svc_context->real_dev_idx, svc_context->mux_dev_idx) == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_ioctl return - interface(adapter) is null\n");
		return -ENODEV;
	}

	if(get_remove_pending(svc_context->real_dev_idx, &remove_pending) < 0){
		pr_warn(NET_DEVICE_NAME " qmi_svc_ioctl return - get remove pending status fail.\n");
		return -ENODEV;
	}

	if(remove_pending == 1){
		pr_warn(NET_DEVICE_NAME " qmi_svc_ioctl return - remove pending status.\n");
		return -ENODEV;
	}
	
	context = svc_context->parent_context;

	if(context == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_ioctl - interface context is null - return\n");
		return -EINVAL;		
	}
	
	real_dev_idx = context->real_dev_idx;
	
	pr_debug(NET_DEVICE_NAME " QMI service ioctl (cmd=0x%x arg=0x%lx)\n", cmd, arg);
	
	if (RMNET_STATUS_REMOVED & context->rmnet_status) {
		pr_err(NET_DEVICE_NAME " ioctl error : device already removed\n");
		return -ENODEV;
	}

	// dispatch command
	switch (cmd) {
	case QMI_SVC_IOCTL_SET_SERVICE :
		
		if (arg == 0) {
			pr_err(NET_DEVICE_NAME " set service error : no service type\n");
			result = -EINVAL;
			break;
		}

		service_type = (u8)arg;

		if (service_type == RMNET_QMI_VERSION_WDS && svc_context->select_adapter == false){

			pr_err(NET_DEVICE_NAME " set service error : not support service type\n");
			result = -ENXIO;
			break;
		}

		// not support control service
		if (svc_context->service_type != 0) {
			pr_err(NET_DEVICE_NAME " set service error : not support service type\n");
			result = -EBADR;
			break;
		}

		// get client id
		result = qmi_ctl_get_client_id(context, service_type, &svc_context->client_id);

		// get client id success
		if (result > 0) {
			svc_context->service_type = service_type;
			
			if (service_type == RMNET_QMI_VERSION_WDS) {

				result = qmi_wds_bind_mux_data_port(context, 0xF0, svc_context->client_id, context->mux_id);

				if(result < 0) {
					pr_err(NET_DEVICE_NAME " client bind mux data port error (mux_id : %d, mux_dev : %d)\n", context->mux_id, context->mux_dev_idx);
				}
			}
		}
		
		break;
	case QMI_SVC_IOCTL_SELECT_ADAPTER :
		
		if (arg >= MAX_RMNET_MUX_DEVICE_NUM) {
			pr_err(NET_DEVICE_NAME " select adapter error : invalid adapter number\n");
			result = -EINVAL;
			break;
		}

		if(get_real_dev(real_dev_idx) == NULL){
			pr_err(NET_DEVICE_NAME " select adapter error : cannot find device\n");
			result = -ENODEV;
			break;
		}

		if(get_mux_dev(real_dev_idx, arg) == NULL){
			pr_err(NET_DEVICE_NAME " select adapter error : cannot find adapter\n");
			result = -ENODEV;
			break;
		}

		mutex_lock(&svc_context->parent_context->cxt_lock);
		remove_list(&svc_context->list);
		mutex_unlock(&svc_context->parent_context->cxt_lock);
		
		svc_context->parent_context = (struct rmnet_context *)get_mux_dev(real_dev_idx, arg)->data[0];
		svc_context->mux_dev_idx = svc_context->parent_context->mux_dev_idx;
		
		mutex_lock(&svc_context->parent_context->cxt_lock);
		add_list_head(&svc_context->parent_context->svc_list_head, &svc_context->list);
		mutex_unlock(&svc_context->parent_context->cxt_lock);

		if(svc_context->parent_context == NULL){
			pr_err(NET_DEVICE_NAME " select adapter error : cannot find adapter context\n");
			svc_context->parent_context = context ; // recover original context;
			result = -ENODEV;
			break;
		}
		
		svc_context->select_adapter = true;
		result = 0;
		
		break;
	default :
		result = -EBADRQC;
		break;
	}

	return result;
}

/*
 * file open handler
 *
 * Parameters
 *   inode : 
 *   filp : file pointer
 *
 * Return value
 *   < 0 : error
 *   = 0 : success
 */
int qmi_svc_open(struct inode *inode, struct file *filp)
{
	int result = 0;
	struct rmnet_context *context = NULL;
	struct qmi_svc_context *svc_context = NULL;
	struct rmnet_context *cur_context = NULL;
	u8 mux_dev_num = 0;
	u8 i = 0;

	// find rmnet context
	context = container_of(inode->i_cdev, struct rmnet_context, cdev);	
	pr_debug(NET_DEVICE_NAME " QMI service open (inode=0x%p filp=0x%p)\n", (void *)inode, (void *)filp);

	if(context == NULL){
		pr_err(NET_DEVICE_NAME " open error : rmnet context is null\n");
		result = -ENODEV;
		goto exit;		
	}
	
	if (RMNET_STATUS_REMOVED & context->rmnet_status) {
		pr_err(NET_DEVICE_NAME " open error : device already removed\n");
		result = -ENODEV;
		goto exit;
	}

	if ((context->rmnet_status & RMNET_STATUS_VERSION) == 0 ||
		(context->rmnet_status & RMNET_STATUS_SYNC) == 0) {
		pr_err(NET_DEVICE_NAME " open error : not yet initialized\n");
		result = -ENODEV;
		goto exit;
	}

	mux_dev_num = get_mux_dev_num(context->real_dev_idx);

#if 0
	if(mux_dev_num != MAX_RMNET_MUX_DEVICE_NUM){
		pr_err(NET_DEVICE_NAME " open error : all mux dev is not init condition.\n");
		result = -ENODEV;
		goto exit;
	}
#endif

	for(i = 0; i < mux_dev_num; i++){
		
		cur_context = (struct rmnet_context *)(get_mux_dev(context->real_dev_idx, i)->data[0]);

		if(cur_context == NULL){
			pr_err(NET_DEVICE_NAME " open error : mux dev [%d] is not init condition - 1.\n", i);
			result = -ENODEV;
			goto exit;			
		}
		
		if(!(cur_context->rmnet_status & RMNET_STATUS_QMI_INIT)){
			
			pr_err(NET_DEVICE_NAME " open error : mux dev [%d] is not init condition - 2.\n", i);
			result = -ENODEV;
			goto exit;
		}
	}

	// allocate service context
	filp->private_data = kmalloc(sizeof(struct qmi_svc_context), GFP_KERNEL);
	if (filp->private_data == NULL) {
		pr_err(NET_DEVICE_NAME " open error : can't alloc memory\n");
		result = -ENOMEM;
		goto exit;
	}

	// initialize service context
	svc_context = filp->private_data;
	svc_context->parent_context = context;
	svc_context->service_type = 0;
	svc_context->client_id = 0;
	initialize_list_head(&svc_context->data_queue);
	svc_context->read_wait_active = 0;
	svc_context->read_wait_queue = NULL;
	svc_context->buffer = NULL;
	svc_context->error_no = 0;
	svc_context->notify_status = WAIT_NOTIFY_STATUS_NONE;
	svc_context->wait_notify_active = 0;
	svc_context->wait_notify_queue = NULL;
	svc_context->select_adapter = false;
	svc_context->real_dev_idx = context->real_dev_idx;
	svc_context->mux_dev_idx = context->mux_dev_idx;
	atomic_set(&svc_context->ref_cnt, 1);

	pr_debug(NET_DEVICE_NAME " add service context 0x%p(file=0x%p)\n", (void *)svc_context, (void *)filp);

	// append service context to context list
	mutex_lock(&context->cxt_lock);
	add_list_head(&context->svc_list_head, &svc_context->list);
	atomic_inc(&svc_context->ref_cnt);
	mutex_unlock(&context->cxt_lock);

	// success
	result = 0;

exit :

	return result;
}

/*
 * file flush handler
 *
 * Parameters
 *   filp : file pointer
 *   id : 
 *
 * Return value
 *   = 0 : success
 */
int qmi_svc_flush(struct file *filp, fl_owner_t id)
{
	int result = 0;
	struct rmnet_context *context = NULL;
	struct qmi_svc_context *svc_context = NULL;
	struct qmi_svc_data_list *data_list = NULL;
	struct list_entry *node, *next_node = NULL;
	void *wait_queue = NULL;
	u8 remove_pending = 0;

	if(filp == NULL){
		return 0;
	}

	svc_context = filp->private_data;

	if(svc_context == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_flush return - service context is null\n");
		return 0;
	}

	if(get_real_dev(svc_context->real_dev_idx) == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_flush return - device is null\n");
		return 0;
	}

	if(get_mux_dev(svc_context->real_dev_idx, svc_context->mux_dev_idx) == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_flush return - interface(adapter) is null\n");
		return 0;
	}

	context = svc_context->parent_context;

	if(context == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_svc_flush return - interface context is null.\n");
		return 0;
	}
	
	pr_debug(NET_DEVICE_NAME " QMI service flush (filp=0x%p context=0x%p read_wait_active=%d)\n", (void *)filp, (void *)svc_context, svc_context->read_wait_active);

	mutex_lock(&context->cxt_lock);

	// remove service context from context list
	remove_list(&svc_context->list);
	atomic_dec(&svc_context->ref_cnt);
	
	// if read response waiting, wakeup thread
	if (svc_context->read_wait_active > 0 &&
		svc_context->read_wait_queue != NULL) {
		svc_context->error_no = -EIO;
		pr_debug(NET_DEVICE_NAME " wake up device(context:0x%p) for read waiting by flush\n", (void *)svc_context);
		svc_context->read_wait_active = 0;
		wait_queue = svc_context->read_wait_queue;
		svc_context->read_wait_queue = NULL;
		mutex_unlock(&context->cxt_lock);
		wake_up_interruptible(wait_queue);
		mutex_lock(&context->cxt_lock);
	}

	// clear remain response messages
	next_node = svc_context->data_queue.next;
	while (next_node != &svc_context->data_queue) {
		node = next_node;
		next_node = node->next;
		remove_list(node);
		data_list = (struct qmi_svc_data_list *)container_of(node, struct qmi_svc_data_list, list);
		pr_debug(NET_DEVICE_NAME " remove remain data list 0x%p\n", (void *)data_list);
		kfree(data_list);
	}

	if(get_remove_pending(svc_context->real_dev_idx, &remove_pending) < 0){
		return 0;
	}

	// release client id
	if (remove_pending == 0 && svc_context->service_type > 0) {
		result = qmi_ctl_release_client_id(context, svc_context->service_type, svc_context->client_id);
		svc_context->service_type = 0;
		svc_context->client_id = 0;
	}

	mutex_unlock(&context->cxt_lock);


	pr_debug(NET_DEVICE_NAME " remove service context 0x%p\n", (void *)svc_context);

	return 0;
}

/*
 * file release handler
 *
 * Parameters
 *   inode : 
 *   filp : file pointer
 *
 * Return value
 *   = 0 : success
 */
int qmi_svc_release(struct inode *inode, struct file *filp)
{
	int result = 0;
	struct qmi_svc_context *svc_context = NULL;

	if(filp == NULL){
		return 0;
	}
	
	svc_context = filp->private_data;

	if(svc_context == NULL){
		return 0;
	}

	printk(KERN_NOTICE "QMI service release (filp=0x%p context=0x%p)\n", (void *)filp, (void *)svc_context);

	// free service context
	if(atomic_dec_return(&svc_context->ref_cnt) == 0){
		pr_debug(NET_DEVICE_NAME " qmi_svc_release - free memory - svc_context\n");
		kfree(svc_context);
	}
	filp->private_data = NULL;

	return result;
}

/*
 * process response message for internal services
 *
 * Parameters
 *   context : rmnet context
 *   buffer : response message buffer
 *   length : response message length
 *
 * Return value
 *   0
 */
int qmi_svc_internal_proc_response(struct rmnet_context *context, void *buffer, int length, u8 service_type)
{
	int status = 0;
	
	if (RMNET_QMI_VERSION_WDS == service_type) {
		status = qmi_interal_wds_proc(context, buffer);
	}

	return status;
}

/*
 * find internal message transactions
 *
 * Parameters
 *   context : rmnet context
 *   buffer : response message buffer
 *   length : response message length
 *
 * Return value
 *   true : found transaction
 *   false : not found transaction
 */
bool qmi_svc_proc_transaction(struct rmnet_context *context, void *buffer, int length)
{
	bool processed = false;
	struct qmi_svc_response_header *header = NULL;
	struct qmi_ctl_transaction_item *tx_item = NULL;
	struct list_entry *entry;
	
	header = (struct qmi_svc_response_header *)buffer;
	mutex_lock(&context->cxt_lock);
	entry = &context->tx_list_head;
	while (entry->next != &context->tx_list_head) {
		entry = entry->next;

		// get transaction item
		tx_item = (struct qmi_ctl_transaction_item *)container_of(entry, struct qmi_ctl_transaction_item, list);

		// if transaction id equals
		if (tx_item->service_type == header->msg_header.service_type &&
			tx_item->client_id == header->msg_header.client_id &&
			tx_item->transaction_id == header->sdu_header.transaction_id) {
			remove_list(&tx_item->list);

			pr_debug(NET_DEVICE_NAME " find transaction item for service 0x%x client id 0x%x transaction id 0x%x\n", 
				tx_item->service_type, tx_item->client_id, tx_item->transaction_id);

			// copy response data
			if (tx_item->buffer)
				memcpy(tx_item->buffer, buffer, length);

			// set qmi result code value
			tx_item->qmi_result = header->qmi_result;
			tx_item->qmi_error = header->qmi_error;

			// set response data length
			tx_item->length = length;

			// wakeup thread
			if (tx_item->wait_queue_head) {
				mutex_unlock(&context->cxt_lock);
				wake_up_interruptible(tx_item->wait_queue_head);
				mutex_lock(&context->cxt_lock);
			}

			processed = true;
			break;
		}
	}
	
	mutex_unlock(&context->cxt_lock);
	return processed;
}

/*
 * process response message for services
 *
 * Parameters
 *   context : rmnet context
 *   buffer : response message buffer
 *   length : response message length
 *
 * Return value
 *   0
 */
int qmi_svc_proc_response(struct rmnet_context *context, void *buffer, int length)
{
	int status = 0;
	//int i = 0;

	struct qmi_svc_response_header *header = NULL;
	struct list_entry *node = NULL;

	struct qmi_svc_context *svc_context = NULL;
	struct qmi_svc_data_list *data_list = NULL;

	struct rmnet_context* real_rmnet_context = NULL;


	status = find_rmnet_context(context, buffer, &real_rmnet_context);

	if(status < 0){
		status = find_rmnet_context_with_svc_context(context, buffer, &real_rmnet_context);

		if(status < 0){
			printk(KERN_WARNING "qmi_svc_proc_response - find real rmnet_context fail\n");
		}else{
			pr_debug(NET_DEVICE_NAME " qmi_svc_proc_response - using svc context - find real rmnet_context success\n");
			context = real_rmnet_context;
		}
	}else{
		context = real_rmnet_context;
	}

	// get header
	header = (struct qmi_svc_response_header *)buffer;

	pr_debug(NET_DEVICE_NAME " service msg read %d bytes(svc-0x%x cid-0x%x txid-0x%x msgid-0x%x)\n", 
		length, header->msg_header.service_type, header->msg_header.client_id,
		header->sdu_header.transaction_id, header->message_id);

	if (qmi_svc_proc_transaction(context, buffer, length)) {
		return status;
	}

	// internal service
	if (header->msg_header.client_id == context->qmi_wds_client_id || 
		header->msg_header.client_id == context->qmi_wds_client_ex || 
		header->msg_header.client_id == 0xFF) {
		status = qmi_svc_internal_proc_response(context, buffer, length, header->msg_header.service_type);
	}

	mutex_lock(&context->cxt_lock);
	node = &context->svc_list_head;

	// retreive opened services
	while (node->next != &context->svc_list_head) {
		node = node->next;
		svc_context = (struct qmi_svc_context *)container_of(node, struct qmi_svc_context, list);

		// match service type
		if (header->msg_header.service_type == svc_context->service_type) {
			// match client id or broadcast
			if (header->msg_header.client_id == svc_context->client_id || header->msg_header.client_id == 0xFF) {
				// allocate response message buffer
				data_list = kmalloc(sizeof(struct qmi_svc_data_list) + length - sizeof(struct qmi_qmux_msg_header), GFP_KERNEL);
				data_list->buffer = (void *)(data_list + 1);
				data_list->length = length - sizeof(struct qmi_qmux_msg_header);
				memcpy(data_list->buffer, (void *)(buffer + sizeof(struct qmi_qmux_msg_header)), length - sizeof(struct qmi_qmux_msg_header));
				pr_debug(NET_DEVICE_NAME " response data 0x%p append to 0x%p\n", (void *)data_list, (void *)svc_context);
				add_list_head(&svc_context->data_queue, &data_list->list);

				// if read thread is waiting, wakeup thread
				if (svc_context->read_wait_active > 0) {
					svc_context->read_wait_active = 0;
					mutex_unlock(&context->cxt_lock);
					wake_up_interruptible(svc_context->read_wait_queue);
					mutex_lock(&context->cxt_lock);
				}
			}
		}
	}
	mutex_unlock(&context->cxt_lock);

	return status;
}

