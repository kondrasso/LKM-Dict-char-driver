CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror


TEST_PREFIX = test
EXAMPLE_PREFIX = example
CLIENT_PREFIX = src/client
DRIVER_PREFIX = src/driver

.PHONY: all clean install uninstall

all: driver client example_client test_stress_typed test_stress_untyped test_error_codes

driver:
			cd $(DRIVER_PREFIX)/ && make
client:
			$(CC) $(CFLAGS) -c $(CLIENT_PREFIX)/client.c
			mv client.o $(CLIENT_PREFIX)/
example_client:
			$(CC) $(CFLAGS) -c $(EXAMPLE_PREFIX)/example_client.c
			mv example_client.o $(EXAMPLE_PREFIX)/
			$(CC) -o $(EXAMPLE_PREFIX)/example_client $(EXAMPLE_PREFIX)/example_client.o $(CLIENT_PREFIX)/client.o
test_stress_typed:
			$(CC) $(CFLAGS) -c $(TEST_PREFIX)/test_stress_typed.c
			mv test_stress_typed.o $(TEST_PREFIX)/
			$(CC) -o $(TEST_PREFIX)/test_stress_typed $(TEST_PREFIX)/test_stress_typed.o $(CLIENT_PREFIX)/client.o
test_stress_untyped:
			$(CC) $(CFLAGS) -c $(TEST_PREFIX)/test_stress_untyped.c
			mv test_stress_untyped.o $(TEST_PREFIX)/
			$(CC) -o $(TEST_PREFIX)/test_stress_untyped $(TEST_PREFIX)/test_stress_untyped.o $(CLIENT_PREFIX)/client.o
test_error_codes:
			$(CC) $(CFLAGS) -c $(TEST_PREFIX)/test_error_codes.c
			mv test_error_codes.o $(TEST_PREFIX)/
			$(CC) -o $(TEST_PREFIX)/test_error_codes $(TEST_PREFIX)/test_error_codes.o $(CLIENT_PREFIX)/client.o
			
clean:
			-rm -f $(TEST_PREFIX)/*.o 
			-rm -f $(TEST_PREFIX)/test_stress_typed
			-rm -f $(TEST_PREFIX)/test_stress_untyped
			-rm -f $(TEST_PREFIX)/test_error_codes
			-rm -f $(EXAMPLE_PREFIX)/*.o
			-rm -f $(EXAMPLE_PREFIX)/example_client
			-rm -f $(CLIENT_PREFIX)/*.o
			cd $(DRIVER_PREFIX)/ && make clean 