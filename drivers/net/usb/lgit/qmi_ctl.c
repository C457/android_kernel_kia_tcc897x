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

#include "config.h"
#include "util.h"
#include "rmnet.h"
#include "qmi_ctl.h"


/*
 * Send control message
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
int qmi_ctl_send_and_forget(struct rmnet_context *context, void *msg, int msglen)
{
	int status = 0;
	u8 remove_pending = 0;

	pr_debug(NET_DEVICE_NAME " send and forget control message\n");

	if(context == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_ctl_send_and_forget - device context is null - return\n");
		return -EINVAL;
	}

	if(get_remove_pending(context->real_dev_idx, &remove_pending) < 0){
		pr_warn(NET_DEVICE_NAME " qmi_ctl_send_and_forget - remove pending check - return\n");
		return -ENODEV;
	}

	if(remove_pending == 1){
		pr_warn(NET_DEVICE_NAME " qmi_ctl_send_and_forget - remove pending condition - return\n");
		return -ENODEV;		
	}

	if (RMNET_STATUS_REMOVED & context->rmnet_status) {
		pr_warn(NET_DEVICE_NAME " qmi_ctl_send_and_forget : device already removed\n");
		return -ENODEV;
	}

	if(get_real_dev(context->real_dev_idx) == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_ctl_send_and_forget - device is null - return\n");
		return -EINVAL;
	}

	
	DBG_DUMP_MEMORY((char *)msg, msglen);

	// send control message
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
 * Send control message and wait response
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
int qmi_ctl_send_and_wait(struct rmnet_context *context, void *msg, int msglen, u8 txid, void *buf, int buflen)
{
	int status = 0;

	struct qmi_ctl_transaction_item *tx_item = NULL;
	u8 remove_pending = 0;

	DECLARE_WAIT_QUEUE_HEAD(wait_queue_head);


	if(context == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_ctl_send_and_wait - device context is null - return\n");
		return -EINVAL;
	}

	if(get_remove_pending(context->real_dev_idx, &remove_pending) < 0){
		pr_warn(NET_DEVICE_NAME " qmi_ctl_send_and_wait - remove pending check - return\n");
		return -ENODEV;
	}

	if(remove_pending == 1){
		pr_warn(NET_DEVICE_NAME " qmi_ctl_send_and_wait - remove pending condition - return\n");
		return -ENODEV;		
	}

	if (RMNET_STATUS_REMOVED & context->rmnet_status) {
		pr_warn(NET_DEVICE_NAME " qmi_ctl_send_and_wait : device already removed\n");
		return -ENODEV;
	}

	if(get_real_dev(context->real_dev_idx) == NULL){
		pr_warn(NET_DEVICE_NAME " qmi_ctl_send_and_wait - device is null - return\n");
		return -EINVAL;
	}
	
	// allocate memory for transaction item
	tx_item = kmalloc(sizeof(struct qmi_ctl_transaction_item), GFP_KERNEL);
	if (tx_item == NULL) {
		pr_err(NET_DEVICE_NAME " control txitem memory alloc error\n");
		status = -ENOMEM;
		goto exit;
	}

	// initialize transaction item
	tx_item->wait_queue_head = &wait_queue_head;
	tx_item->service_type = RMNET_QMI_VERSION_CTL;
	tx_item->client_id = 0;
	tx_item->transaction_id = txid;
	tx_item->buffer = buf;
	tx_item->length = 0;
	tx_item->qmi_result = 0;
	tx_item->qmi_error = 0;

	// append transaction item
	mutex_lock(&context->cxt_lock);
	add_list_tail(&context->tx_list_head, &tx_item->list);
	mutex_unlock(&context->cxt_lock);

	pr_debug(NET_DEVICE_NAME " send and wait control message(txid:0x%x)\n", txid);
	DBG_DUMP_MEMORY((char *)msg, msglen);

	// send control message
	status = usb_control_msg(context->udev,
							 usb_sndctrlpipe(context->udev, 0),
							 USB_CDC_SEND_ENCAPSULATED_COMMAND,
							 USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
							 0,
							 context->intf->cur_altsetting->desc.bInterfaceNumber,
							 msg,
							 msglen,
							 RMNET_RESPONSE_TIMEOUT_MS);

	// control message send failure
	if (status < 0) {
		pr_err(NET_DEVICE_NAME " usb control message send error %d\n", status);
        mutex_lock(&context->cxt_lock);
		remove_list(&tx_item->list);
        mutex_unlock(&context->cxt_lock);
		goto exit;
	}

	// sleep few seconds for response
	wait_event_interruptible_timeout(wait_queue_head, tx_item->length != 0, 1*HZ);

	// remove transaction item in list
	mutex_lock(&context->cxt_lock);
	remove_list(&tx_item->list);
	mutex_unlock(&context->cxt_lock);

	pr_debug(NET_DEVICE_NAME " control wakeup from sleep - buffer received %d bytes\n", tx_item->length);

	// set status
	status = tx_item->length;

exit :

	// free transaction item
	if (tx_item)
		kfree(tx_item);

	return status;
}

/*
 * QMI_CTL_GET_VERSION_INFO
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   < 0 : failure reason ERROR
 *   = 0 : failure reason TIMEOUT
 *   > 0 : success
 */
int qmi_ctl_get_version_info(struct rmnet_context *context)
{
	int status = 0;

	char *buffer = NULL, *read_offset = NULL;
	u8 transaction_id = 0;
	struct qmi_ctl_get_version_info_req *msg = NULL;

	struct qmi_ctl_response_header *response = NULL;
	struct qmi_ctl_qmux_version_list *version_list = NULL;
	struct qmi_ctl_qmux_version_info *version_info = NULL;
	int i;

	pr_debug(NET_DEVICE_NAME " qmi get version info\n");

	// get control transaction id
	transaction_id = qmi_ctl_get_txid(context);

	// allocate memory for send + response message buffer
	buffer = kmalloc(sizeof(struct qmi_ctl_get_version_info_req) + RMNET_RESPONSE_BUFFER_LEN, GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, sizeof(struct qmi_ctl_get_version_info_req));

	// initialize send message
	msg = (struct qmi_ctl_get_version_info_req *)buffer;
	msg->message_id = QMI_CTL_GET_VERSION_INFO;
	msg->length = 0x0; // no tlvs

	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xb + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_CTL;
	msg->msg_header.client_id = 0;

	read_offset = buffer + sizeof(struct qmi_ctl_get_version_info_req);

	// send message and wait response
	status = qmi_ctl_send_and_wait(context, 
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1,
					msg->sdu_header.transaction_id,
					read_offset,
					RMNET_RESPONSE_BUFFER_LEN);

	// send failure
	if (status < 0) {
		pr_err(NET_DEVICE_NAME " qmi get version error\n");
		goto exit;
	} else if (status == 0) {
		pr_debug(NET_DEVICE_NAME " qmi get version - no response\n");
		goto exit;
	}

	// get response message buffer
	response = (struct qmi_ctl_response_header *)read_offset;

	// qmi result error
	if (response->qmi_result != 0) {
		pr_err(NET_DEVICE_NAME " qmi get version result error\n");
		status = -1;
		goto exit;
	}

	version_list = (struct qmi_ctl_qmux_version_list *)(response + 1);
	version_info = (struct qmi_ctl_qmux_version_info *)(version_list + 1);

	// retreive version informations
	for (i = 0 ; i < version_list->num_instances ; i++) {
		if (version_info->qmi_svc_type < RMNET_QMI_VERSION_MAX) {
			context->qmi_version[version_info->qmi_svc_type].major_ver = version_info->major_ver;
			context->qmi_version[version_info->qmi_svc_type].minor_ver = version_info->minor_ver;
		}
		version_info = (struct qmi_ctl_qmux_version_info *)(version_info + 1);
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}

/*
 * QMI_CTL_GET_CLIENT_ID
 *
 * Parameters
 *   context : rmnet context
 *   service_type : service type
 *   client_id : responsed client id
 *
 * Return value
 *   < 0 : failure reason ERROR
 *   = 0 : failure reason TIMEOUT
 *   > 0 : success
 */
int qmi_ctl_get_client_id(struct rmnet_context *context, u8 service_type, u8 *client_id)
{
	int status = 0;

	char *buffer = NULL, *read_offset = NULL;
	u8 transaction_id = 0;
	struct qmi_ctl_get_client_id_req *msg = NULL;

	struct qmi_ctl_response_header *response = NULL;
	struct qmi_ctl_assigned_client_id *assigned_cid = NULL;

	pr_debug(NET_DEVICE_NAME " qmi get client id(service 0x%x)\n", service_type);

	// get control transaction id
	transaction_id = qmi_ctl_get_txid(context);

	// allocate memory for send + response message buffer
	buffer = kmalloc(sizeof(struct qmi_ctl_get_client_id_req) + RMNET_RESPONSE_BUFFER_LEN, GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, sizeof(struct qmi_ctl_get_client_id_req));

	// initialize send message
	msg = (struct qmi_ctl_get_client_id_req *)buffer;
	msg->message_id = QMI_CTL_GET_CLIENT_ID;
	msg->length = 0x4;

	msg->tlv_type = 0x01;
	msg->tlv_length = 0x0001;
	msg->qmi_svc_type = service_type;

	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xb + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_CTL;
	msg->msg_header.client_id = 0;

	read_offset = buffer + sizeof(struct qmi_ctl_get_client_id_req);

	// send message and wait response
	status = qmi_ctl_send_and_wait(context, 
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1,
					msg->sdu_header.transaction_id,
					read_offset,
					RMNET_RESPONSE_BUFFER_LEN);

	// send failure
	if (status < 0) {
		pr_err(NET_DEVICE_NAME " qmi get client id error\n");
		goto exit;
	} else if (status == 0) {
		pr_debug(NET_DEVICE_NAME " qmi get client id - no response\n");
		goto exit;
	}

	// get response message buffer
	response = (struct qmi_ctl_response_header *)read_offset;

	// qmi result error
	if (response->qmi_result != 0) {
		pr_err(NET_DEVICE_NAME " qmi get client id result error\n");
		status = -1;
		goto exit;
	}

	assigned_cid = (struct qmi_ctl_assigned_client_id *)(response + 1);

	// response message error
	if (assigned_cid->qmi_svc_type != service_type) {
		pr_err(NET_DEVICE_NAME " get client id error - service type mismatched\n");
		status = -1;
		goto exit;
	}

	// set out parameter
	*client_id = assigned_cid->client_id;

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}

/*
 * QMI_CTL_RELEASE_CLIENT_ID
 *
 * Parameters
 *   context : rmnet context
 *   service_type : service type
 *   client_id : client id
 *
 * Return value
 *   < 0 : failure reason ERROR
 *   = 0 : failure reason TIMEOUT
 *   > 0 : success
 */
int qmi_ctl_release_client_id(struct rmnet_context *context, u8 service_type, u8 client_id)
{
	int status = 0;

	char *buffer = NULL;
	u8 transaction_id = 0;
	struct qmi_ctl_release_client_id_req *msg = NULL;

	pr_debug(NET_DEVICE_NAME " qmi release client id(service 0x%x, cid 0x%x)\n", service_type, client_id);

	// get control transaction id
	transaction_id = qmi_ctl_get_txid(context);

	// allocate memory for send message buffer
	buffer = kmalloc(sizeof(struct qmi_ctl_release_client_id_req), GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, sizeof(struct qmi_ctl_release_client_id_req));

	// initialize send message
	msg = (struct qmi_ctl_release_client_id_req *)buffer;
	msg->message_id = QMI_CTL_RELEASE_CLIENT_ID;
	msg->length = 0x5;

	msg->tlv_type = 0x01;
	msg->tlv_length = 0x02;
	msg->qmi_svc_type = service_type;
	msg->client_id = client_id;

	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xb + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_CTL;
	msg->msg_header.client_id = 0;

	// send message and no wait response
	status = qmi_ctl_send_and_forget(context, 
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1);

	// send failure
	if (status < 0) {
		pr_err(NET_DEVICE_NAME " qmi release client id error\n");
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}

/*
 * QMI_CTL_SYNC
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   < 0 : failure reason ERROR
 *   = 0 : failure reason TIMEOUT
 *   > 0 : success
 */
int qmi_ctl_sync(struct rmnet_context *context)
{
	int status = 0;

	char *buffer = NULL, *read_offset = NULL;
	u8 transaction_id = 0;
	struct qmi_ctl_sync_req *msg = NULL;

	struct qmi_ctl_response_header *response = NULL;

	pr_debug(NET_DEVICE_NAME " qmi sync\n");

	// get control transaction id
	transaction_id = qmi_ctl_get_txid(context);

	// allocate memory for send + response message buffer
	buffer = kmalloc(sizeof(struct qmi_ctl_sync_req) + RMNET_RESPONSE_BUFFER_LEN, GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, sizeof(struct qmi_ctl_sync_req));

	// initialize send message
	msg = (struct qmi_ctl_sync_req *)buffer;
	msg->message_id = QMI_CTL_SYNC;
	msg->length = 0x0; // no tlvs

	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xb + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_CTL;
	msg->msg_header.client_id = 0;

	read_offset = buffer + sizeof(struct qmi_ctl_sync_req);

	// send message and wait response
	status = qmi_ctl_send_and_wait(context, 
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1,
					msg->sdu_header.transaction_id,
					read_offset,
					RMNET_RESPONSE_BUFFER_LEN);

	// send failure
	if (status < 0) {
		pr_err(NET_DEVICE_NAME " qmi sync error\n");
		goto exit;
	} else if (status == 0) {
		pr_debug(NET_DEVICE_NAME " qmi sync - no response\n");
		goto exit;
	}

	// get response message buffer
	response = (struct qmi_ctl_response_header *)read_offset;

	// qmi result error
	if (response->qmi_result != 0) {
		pr_err(NET_DEVICE_NAME " qmi sync result error\n");
		status = -1;
		goto exit;
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}

/*
 * QMI_CTL_SET_INSTANCE_ID
 *
 * Parameters
 *   context : rmnet context
 *
 * Return value
 *   < 0 : failure reason ERROR
 *   = 0 : failure reason TIMEOUT
 *   > 0 : success
 */
int qmi_ctl_set_instance_id(struct rmnet_context *context)
{
	int status = 0;

	char *buffer = NULL, *read_offset = NULL;
	u8 transaction_id = 0;
	struct qmi_ctl_set_instance_id_req *msg = NULL;

	struct qmi_ctl_response_header *response = NULL;

	pr_debug(NET_DEVICE_NAME " qmi set instance id\n");

	// get control transaction id
	transaction_id = qmi_ctl_get_txid(context);

	// allocate memory for send + response message buffer
	buffer = kmalloc(sizeof(struct qmi_ctl_sync_req) + RMNET_RESPONSE_BUFFER_LEN, GFP_KERNEL);
	if (buffer == NULL) {
		pr_err(NET_DEVICE_NAME " memory allocation error\n");
		status = -ENOMEM;
		goto exit;
	}

	memset(buffer, 0, sizeof(struct qmi_ctl_set_instance_id_req));

	// initialize send message
	msg = (struct qmi_ctl_set_instance_id_req *)buffer;
	msg->message_id = QMI_CTL_SET_INSTANCE_ID;
	msg->length = sizeof(struct qmi_ctl_host_driver_instance);
	
	msg->host_driver_instance.type = 0x1;
	msg->host_driver_instance.length = 0x1;
	msg->host_driver_instance.host_driver_instance = 0x0;

	msg->sdu_header.control_flags = 0x0;
	msg->sdu_header.transaction_id = transaction_id;

	msg->msg_header.if_type = 0x1;
	msg->msg_header.length = 0xb + msg->length;
	msg->msg_header.control_flags = 0x0;
	msg->msg_header.service_type = RMNET_QMI_VERSION_CTL;
	msg->msg_header.client_id = 0;

	read_offset = buffer + sizeof(struct qmi_ctl_sync_req);

	// send message and wait response
	status = qmi_ctl_send_and_wait(context, 
					(struct qmi_qmux_msg_header *)msg, 
					msg->msg_header.length + 1,
					msg->sdu_header.transaction_id,
					read_offset,
					RMNET_RESPONSE_BUFFER_LEN);

	// send failure
	if (status < 0) {
		pr_err(NET_DEVICE_NAME " qmi set instance id error\n");
		goto exit;
	} else if (status == 0) {
		pr_debug(NET_DEVICE_NAME " qmi set instance id - no response\n");
		goto exit;
	}

	// get response message buffer
	response = (struct qmi_ctl_response_header *)read_offset;

	// qmi result error
	if (response->qmi_result != 0) {
		pr_err(NET_DEVICE_NAME " qmi set instance id result error\n");
		status = -1;
		goto exit;
	}

exit :

	// free message buffer
	if (buffer) kfree(buffer);

	return status;
}

/*
 * process response message for control service
 *
 * Parameters
 *   context : rmnet context
 *   buffer : response message buffer
 *   length : response message length
 *
 * Return value
 *   0
 */
int qmi_ctl_proc_response(struct rmnet_context *context, void *buffer, int length)
{
	int status = 0;

	struct qmi_ctl_response_header *header = NULL;
	struct qmi_ctl_transaction_item *tx_item = NULL;
	struct list_entry *entry;

	struct rmnet_context* real_rmnet_context = NULL;

	// get qmux message header
	header = (struct qmi_ctl_response_header *)buffer;


	status = find_rmnet_context(context, buffer, &real_rmnet_context);

	if(status < 0){

		status = find_rmnet_context_with_svc_context(context, buffer, &real_rmnet_context);

		if(status < 0){
			printk(KERN_WARNING "qmi_ctl_proc_response - find real rmnet_context fail [txid : 0x%x] [ctl_flg : 0x%x] [msg_type : 0x%x]\n", 
				header->sdu_header.transaction_id,
				header->sdu_header.control_flags,
				header->message_id
				);	
		}else{
			pr_debug(NET_DEVICE_NAME " qmi_ctl_proc_response - using svc context - find real rmnet_context success\n");
			context = real_rmnet_context;
		}
	}else{
		context = real_rmnet_context;
	}


	// get qmux message header
	header = (struct qmi_ctl_response_header *)buffer;
	
	pr_debug(NET_DEVICE_NAME " control msg read %d bytes(svc-0x%x cid-0x%x txid-0x%x msgid-0x%x)\n", 
		length, header->msg_header.service_type, header->msg_header.client_id,
		header->sdu_header.transaction_id, header->message_id);

	// entry is list head
	mutex_lock(&context->cxt_lock);
	entry = &context->tx_list_head;

	while (entry->next != &context->tx_list_head) {
		entry = entry->next;

		// get transaction item
		tx_item = (struct qmi_ctl_transaction_item *)container_of(entry, struct qmi_ctl_transaction_item, list);

		// if transaction id equals
		if (tx_item->service_type == RMNET_QMI_VERSION_CTL && 
			tx_item->transaction_id == header->sdu_header.transaction_id) {
			remove_list(&tx_item->list);

			pr_debug(NET_DEVICE_NAME " find transaction item for service 0x%x client id 0x%x transaction id 0x%x\n", 
				tx_item->service_type, tx_item->client_id, tx_item->transaction_id);

			// copy response data
			if (tx_item->buffer)
				memcpy(tx_item->buffer, buffer, length);

			// set response data length
			tx_item->length = length;

			// set qmi result code value
			tx_item->qmi_result = header->qmi_result;
			tx_item->qmi_error = header->qmi_error;

			// wakeup thread
			if (tx_item->wait_queue_head) {
				mutex_unlock(&context->cxt_lock);	
				wake_up_interruptible(tx_item->wait_queue_head);
				mutex_lock(&context->cxt_lock);
			}
			
			break;
		}
	}

	mutex_unlock(&context->cxt_lock);
	return status;
}
