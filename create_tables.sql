CREATE TABLE IF NOT EXISTS information(
	name VARCHAR(255) PRIMARY KEY,
	score INT DEFAULT NULL,
	status TINYINT DEFAULT NULL,

    INDEX idx_info_name_status (name, status)
);

CREATE TABLE IF NOT EXISTS user(
	id INT PRIMARY KEY AUTO_INCREMENT,
	name VARCHAR(255) NOT NULL,
	password VARCHAR(255) NOT NULL,
	phone VARCHAR(255) DEFAULT NULL,
	date DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    INDEX idx_user_name_password (name, password)
);