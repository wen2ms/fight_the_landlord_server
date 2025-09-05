#### 1. 报文格式

```proto
message Information {
    string user_name = 1;
    string room_name = 2;
    string data1 = 3;
    string data2 = 4;
    string data3 = 5;
    RequestCode reqcode = 6;
    ResponseCode rescode = 7;
}
```

#### 2. 通信流程

##### 2.1 建立连接和非对称加密

首先客户端向服务端发起连接连接请求，然后服务端收到后，就会将`RSA`已经生成好的`public_key`，长度为`RsaCrypto::kBits2k = 2048`，经过数字签名的`public_key`发送给客户端，这些数据都是通过`Base64`编码之后的。

`data1 = public_key; data2 = sign; rescode = RSA_DISTRIBUTION;` 。

##### 2.2 对称加密

客户端收到服务端的消息后，解码并验证数字签名，成功后向服务端发送使用`RSA`公钥加密后的`AES`私钥，以及`AES`私钥通过`QCryptographicHash::Sha224`得到的`hash`。

`data1 = rsa.pub_key_encrypt(aes_key_); data2 = hash; reqcode = AES_DISTRIBUTION;`。

服务端收到客户端的消息后，同样使用`HashType::kSha224`验证`hash`并用`RSA private_key`解密后得到双方用于对称加密的`AES`密钥，最后回复给客户端验证成功，规定之后的加密都是用`AesCrypto::kAesCbc256`进行加密。

`rescode = AES_VERIFY_OK;`

##### 2.3 登录与注册

当建立好加密连接后，客户端将注册或登录的参数发送给服务端。注册或登录时会首先都会通过正则校验检验信息是否合法，然后将信息发送给服务端。

登录：`user_name = user_name; data1 = hash(password); reqcode = USER_LOGIN;`

注册：`user_name = user_name; data1 = hash(password); data2 = phone; reqcode = REGISTER;`

这里都是通过`QCryptographicHash::hash(password, QCryptographicHash::Sha224)`进行`hash`。

##### 2.4 更新数据库

服务端接收到注册或者登录消息，解码后更新数据库。

对于登录，首先通过

```sql
SELECT user.name FROM user JOIN information ON user.name = information.name AND information.status = 0 WHERE
user.name = user_name AND user.password = password;
```

查看玩家是否注册并没有登录，如果成功，就开启事务，更新数据库为`UPDATE information SET status = 1 WHERE name = user_name;`成功后就`commit`更新事务并发送消息`rescode = LOGIN_OK;`给客户端，否则就`rollback`，并发送`rescode = FAILED; data1 = "Login failed...";`。

对于注册，





