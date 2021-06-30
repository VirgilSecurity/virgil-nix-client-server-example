package org.virgil.notification;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.util.Log;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;

import org.virgil.notification.Constants;
import org.virgil.notification.DecryptedNotification;

import java.lang.ref.WeakReference;

import org.qtproject.qt5.android.bindings.QtService;

import com.virgilsecurity.android.virgil.R;

public class PushHandlerService extends QtService
{
    private static final String TAG = "PushHandlerService";

    private static final String MESSAGE_CHANNEL_ID = "notification_channel_message";

    private Looper serviceLooper;
    private ServiceHandler serviceHandler;

    private static native DecryptedNotification decryptNotification(String recipientJid, String senderJid, String ciphertext);

    /**
     * Handler that receives messages from the thread.
     */
    private final class ServiceHandler extends Handler {

        private WeakReference<Context> mContext;

        public ServiceHandler(Context context, Looper looper) {
            super(looper);
            this.mContext = new WeakReference<Context>(context);
        }

        @Override
        public void handleMessage(Message message) {
            Log.d(TAG, "Handle message");
            String messageId = message.getData().getString(Constants.PUSH.MESSAGE_ID);
            String senderId = message.getData().getString(Constants.PUSH.SENDER_ID);
            String senderJid = message.getData().getString(Constants.PUSH.SENDER_JID);
            String recipientJid = message.getData().getString(Constants.PUSH.RECIPIENT_JID);
            String ciphertext = message.getData().getString(Constants.PUSH.CIPHERTEXT);


            //
            //  Try to decrypt encrypted message.
            //
            DecryptedNotification decryptedNotification = decryptNotification(recipientJid, senderJid, ciphertext);

            //
            //  Show notification or ignore it.
            //
            switch(decryptedNotification.getResult()) {
                case DECRYPTED:
                    showNotification(messageId.hashCode(), decryptedNotification.getTitle(), decryptedNotification.getBody());
                    break;

                case FAILED:
                    showNotification(messageId.hashCode(), "New Message", "Encrypted");
                    break;

                case SKIPPED:
                    break;

            }

            //
            //  Stop the working thread.
            //
            stopSelf(message.arg1);
        }

        private void showNotification(int notificationId, String title, String body) {
            Log.d(TAG, "Got message: " + body);

            Context context = mContext.get();
            NotificationCompat.Builder builder = new NotificationCompat.Builder(context, MESSAGE_CHANNEL_ID)
                    .setSmallIcon(R.drawable.icon)
                    .setAutoCancel(true)
                    .setContentTitle(title)
                    .setContentText(body)
                    .setPriority(NotificationManager.IMPORTANCE_DEFAULT);

            NotificationManagerCompat notificationManager = NotificationManagerCompat.from(context);
            notificationManager.notify(notificationId, builder.build());
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.O)
    private void createNotificationChannel() {
        Log.d(TAG, "Create notification channel: " + MESSAGE_CHANNEL_ID);

        CharSequence name = getString(R.string.message_channel_name);
        String description = getString(R.string.message_channel_description);
        int importance = NotificationManager.IMPORTANCE_DEFAULT;
        NotificationChannel channel = new NotificationChannel(MESSAGE_CHANNEL_ID, name, importance);
        channel.setDescription(description);
        //
        //  Register the channel with the system; you can't change the importance
        //  or other notification behaviors after this.
        //
        NotificationManager notificationManager = getSystemService(NotificationManager.class);
        notificationManager.createNotificationChannel(channel);
    }

    @Override
    public void onCreate() {
        super.onCreate();

        createNotificationChannel();

        HandlerThread thread = new HandlerThread("ServiceStartArguments", Process.THREAD_PRIORITY_BACKGROUND);
        thread.start();
        serviceLooper = thread.getLooper();
        serviceHandler = new ServiceHandler(getApplicationContext(), serviceLooper);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "Service starting...");

        //
        //  For each start request, send a message to start a job and deliver the
        //  start ID so we know which request we're stopping when we finish the job.
        //
        Message message = serviceHandler.obtainMessage();
        message.arg1 = startId;

        Bundle data = new Bundle();
        data.putString(Constants.PUSH.MESSAGE_ID, intent.getStringExtra(Constants.PUSH.MESSAGE_ID));
        data.putString(Constants.PUSH.SENDER_ID, intent.getStringExtra(Constants.PUSH.SENDER_ID));
        data.putString(Constants.PUSH.SENDER_JID, intent.getStringExtra(Constants.PUSH.SENDER_JID));
        data.putString(Constants.PUSH.RECIPIENT_JID, intent.getStringExtra(Constants.PUSH.RECIPIENT_JID));
        data.putString(Constants.PUSH.CIPHERTEXT, intent.getStringExtra(Constants.PUSH.CIPHERTEXT));
        message.setData(data);
        serviceHandler.sendMessage(message);

        //
        //  If we get killed, after returning from here, restart.
        //
        return START_REDELIVER_INTENT;
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "Service done.");
    }
}
