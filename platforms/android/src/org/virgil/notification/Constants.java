package org.virgil.notification;

public interface Constants {
    /* Broadcast fields */
    public interface PUSH {
        String HANDLE_PUSH_ACTION = "org.virgil.messenger.HANDLE_PUSH_ACTION";

        String CHANNEL_ID = "channel_id";
        String MESSAGE_ID = "message_id";
        String SENDER_ID = "sender_id";
        String SENDER_JID = "sender_jid";
        String RECIPIENT_JID = "recipient_jid";
        String CIPHERTEXT = "ciphertext";
    }
}
