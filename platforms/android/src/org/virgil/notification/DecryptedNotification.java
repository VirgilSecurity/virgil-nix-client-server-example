package org.virgil.notification;

public class DecryptedNotification {
    private Result result;
    private String title;
    private String body;

    public enum Result {
        DECRYPTED,
        FAILED,
        SKIPPED
    }

    public static DecryptedNotification createDecrypted(String title, String body) {
        return new DecryptedNotification(Result.DECRYPTED, title, body);
    }

    public static DecryptedNotification createFailed() {
        return new DecryptedNotification(Result.FAILED, "", "");
    }

    public static DecryptedNotification createSkipped() {
        return new DecryptedNotification(Result.SKIPPED, "", "");
    }

    private DecryptedNotification(Result result, String title, String body) {
        this.result = result;
        this.title = title;
        this.body = body;
    }

    public Result getResult() {
        return this.result;
    }

    public String getTitle() {
        return this.title;
    }

    public String getBody() {
        return this.body;
    }
}
