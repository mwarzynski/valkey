import redis
import time
import random
import string


def get_set_client():
    # Connect to Redis
    client = redis.Redis(host="localhost", port=6379, db=0)

    # Function to generate a random string of specified length
    def generate_random_string(length=8):
        letters = string.ascii_letters
        return "".join(random.choice(letters) for _ in range(length))

    # Initialize set_keys with existing keys in Redis
    set_keys = [key.decode("utf-8") for key in client.keys("*")]
    print(f"KEYS * -> {set_keys}")

    # Counter to determine whether to perform GET or SET
    operation_counter = 0

    try:
        while True:
            if operation_counter % 11 == 0:  # Every 11th operation will be a SET
                # Generate random key and value
                key = generate_random_string(8)
                # value_length = random.randint(10, 20)
                value = generate_random_string(10)

                # Perform SET command with random key and value
                client.set(key, value)
                print(f"SET {key} -> {value}")

                # Add the new key to the list of set keys
                set_keys.append(key)
            else:
                if random.random() < 0.05:  # 5% chance to GET a non-existing key
                    non_existent_key = generate_random_string(8)
                    result = client.get(non_existent_key)
                    print(f"GET {non_existent_key} -> {result if result else 'Key not found'}")
                else:
                    # Perform GET command for a random previously set key if available
                    if set_keys:
                        random_key = random.choice(set_keys)
                        result = client.get(random_key)
                        if result:
                            print(f"GET {random_key} -> {result.decode('utf-8')}")
                        else:
                            print(f"GET {random_key} -> Key not found")
                    else:
                        print("No keys available for GET operation.")

            # Increment the operation counter
            operation_counter += 1

            # Wait for 100ms before the next iteration
            time.sleep(0.1)

    except KeyboardInterrupt:
        print("Process interrupted by user.")


def event_handler_client():
    def event_handler(message):
        print(f"Event received: {message}")

    client = redis.Redis(host="localhost", port=6379, db=0, decode_responses=True)
    pubsub = client.pubsub()

    # Subscribe to all keyspace and key event notifications for database 0
    pubsub.psubscribe(**{"*": event_handler, "*": event_handler})

    # Start listening in a background thread
    print("Listening for all events in keyspace 0...")
    pubsub_thread = pubsub.run_in_thread(sleep_time=0.001)

    try:
        # Keep the main thread alive
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Shutting down...")
        # Stop the pubsub thread on exit
        pubsub_thread.stop()


if __name__ == "__main__":
    get_set_client()
