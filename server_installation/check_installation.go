package main

import (
	"context"
	"database/sql"
	"fmt"
	"log"
	"net"
	"os"

	"github.com/go-redis/redis/v8"
	_ "github.com/go-sql-driver/mysql"
	_ "github.com/lib/pq" // Import the PostgreSQL driver
	"github.com/segmentio/kafka-go"
	"github.com/streadway/amqp"
)

const (
	red   = "\033[31m"
	green = "\033[32m"
	reset = "\033[0m"
)

func main() {
	// RabbitMQ
	rabbitmqHost := getEnv("RABBITMQ_HOST", "localhost")
	rabbitmqPort := getEnv("RABBITMQ_PORT", "5672")
	rabbitmqUser := getEnv("RABBITMQ_USER", "guest")
	rabbitmqPass := getEnv("RABBITMQ_PASS", "guest")
	rabbitmqURL := fmt.Sprintf("amqp://%s:%s@%s:%s/", rabbitmqUser, rabbitmqPass, rabbitmqHost, rabbitmqPort)
	checkRabbitMQ(rabbitmqURL)

	// PostgreSQL
	postgresHost := getEnv("POSTGRES_HOST", "localhost")
	postgresPort := getEnv("POSTGRES_PORT", "5432")
	postgresUser := getEnv("POSTGRES_USER", "testuser")
	postgresPassword := getEnv("POSTGRES_PASSWORD", "Password123!")
	postgresDBName := getEnv("POSTGRES_DB", "testdb")
	postgresSSLMode := getEnv("POSTGRES_SSLMODE", "disable")
	checkPostgres(postgresHost, postgresPort, postgresUser, postgresPassword, postgresDBName, postgresSSLMode)

	// Redis
	redisHost := getEnv("REDIS_HOST", "localhost")
	redisPort := getEnv("REDIS_PORT", "6379")
	redisAddress := fmt.Sprintf("%s:%s", redisHost, redisPort)
	checkRedis(redisAddress)

	// Kafka
	kafkaHost := getEnv("KAFKA_BROKER", "localhost")
	kafkaPort := getEnv("KAFKA_PORT", "9092")
	kafkaBroker := fmt.Sprintf("%s:%s", kafkaHost, kafkaPort)
	kafkaTopic := getEnv("KAFKA_TOPIC", "test-topic")
	checkKafka(kafkaBroker, kafkaTopic)

	// MySQL
	mysqlHost := getEnv("MYSQL_HOST", "localhost")
	mysqlPort := getEnv("MYSQL_PORT", "3306")
	mysqlUser := getEnv("MYSQL_USER", "testuser")
	mysqlPassword := getEnv("MYSQL_PASSWORD", "Password123!")
	mysqlDBName := getEnv("MYSQL_DB", "testdb")
	checkMySQL(mysqlHost, mysqlPort, mysqlUser, mysqlPassword, mysqlDBName)

	// Zookeeper
	zookeeperHost := getEnv("ZOOKEEPER_HOST", "localhost")
	zookeeperPort := getEnv("ZOOKEEPER_PORT", "2181")
	checkZookeeper(zookeeperHost, zookeeperPort)
}

func getEnv(key, defaultValue string) string {
	value, exists := os.LookupEnv(key)
	if exists {
		log.Printf("Using environment variable %s=%s", key, value)
		return value
	}
	log.Printf("Environment variable %s not set, using default value %s", key, defaultValue)
	return defaultValue
}

func checkRabbitMQ(url string) {
	conn, err := amqp.Dial(url)
	if err != nil {
		log.Printf("%sRabbitMQ connection failed: %v%s", red, err, reset)
	} else {
		defer conn.Close()
		log.Printf("%sRabbitMQ is up and running%s", green, reset)
	}
}

func checkPostgres(host, port, user, password, dbname, sslmode string) {
	connStr := fmt.Sprintf("host=%s port=%s user=%s password=%s dbname=%s sslmode=%s",
		host, port, user, password, dbname, sslmode)
	db, err := sql.Open("postgres", connStr)
	if err != nil {
		log.Printf("%sPostgreSQL connection failed: %v%s", red, err, reset)
		return
	}
	defer db.Close()

	if err := db.Ping(); err != nil {
		log.Printf("%sPostgreSQL ping failed: %v%s", red, err, reset)
	} else {
		log.Printf("%sPostgreSQL is up and running%s", green, reset)
	}
}

func checkRedis(address string) {
	ctx := context.Background()
	rdb := redis.NewClient(&redis.Options{
		Addr: address,
	})

	if err := rdb.Ping(ctx).Err(); err != nil {
		log.Printf("%sRedis connection failed: %v%s", red, err, reset)
	} else {
		log.Printf("%sRedis is up and running%s", green, reset)
	}
}

func checkKafka(broker, topic string) {
	conn, err := kafka.DialLeader(context.Background(), "tcp", broker, topic, 0)
	if err != nil {
		log.Printf("%sKafka connection failed: %v%s", red, err, reset)
		return
	}
	defer conn.Close()
	log.Printf("%sKafka is up and running%s", green, reset)
}

func checkMySQL(host, port, user, password, dbname string) {
	dsn := fmt.Sprintf("%s:%s@tcp(%s:%s)/%s", user, password, host, port, dbname)
	db, err := sql.Open("mysql", dsn)
	if err != nil {
		log.Printf("%sMySQL connection failed: %v%s", red, err, reset)
		return
	}
	defer db.Close()

	if err := db.Ping(); err != nil {
		log.Printf("%sMySQL ping failed: %v%s", red, err, reset)
	} else {
		log.Printf("%sMySQL is up and running%s", green, reset)
	}
}

func checkZookeeper(host, port string) {
	address := net.JoinHostPort(host, port)
	conn, err := net.Dial("tcp", address)
	if err != nil {
		log.Printf("%sZookeeper connection failed: %v%s", red, err, reset)
	} else {
		defer conn.Close()
		log.Printf("%sZookeeper is up and running%s", green, reset)
	}
}
