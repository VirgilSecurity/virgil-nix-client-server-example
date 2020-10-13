CREATE TABLE messages (
	id TEXT NOT NULL PRIMARY KEY,
	timestamp TEXT NOT NULL,
	author_id TEXT NOT NULL,
	status INT NOT NULL,
	body TEXT,
	FOREIGN KEY(author_id) REFERENCES contacts(id)
);

CREATE INDEX messages_idx_author_id ON messages(author_id);

CREATE INDEX messages_idx_status ON messages(status);
