TEST_PREFIX = test
EXAMPLE_PREFIX = example
CLIENT_PREFIX = src/client
DRIVER_PREFIX = src/driver

.PHONY: all clean install uninstall

all: driver client example_client test_stress test_error_codes

driver:
			cd $(DRIVER_PREFIX)/ && make
client:
			gcc -c $(CLIENT_PREFIX)/client.c
			mv client.o $(CLIENT_PREFIX)/
example_client:
			gcc -c $(EXAMPLE_PREFIX)/example_client.c
			mv example_client.o $(EXAMPLE_PREFIX)/
			gcc -o $(EXAMPLE_PREFIX)/example_client $(EXAMPLE_PREFIX)/example_client.o $(CLIENT_PREFIX)/client.o
test_stress:
			gcc -c -w $(TEST_PREFIX)/test_stress.c
			mv test_stress.o $(TEST_PREFIX)/
			gcc -o $(TEST_PREFIX)/test_stress $(TEST_PREFIX)/test_stress.o $(CLIENT_PREFIX)/client.o
test_error_codes:
			gcc -c $(TEST_PREFIX)/test_error_codes.c
			mv test_error_codes.o $(TEST_PREFIX)/
			gcc -o $(TEST_PREFIX)/test_error_codes $(TEST_PREFIX)/test_error_codes.o $(CLIENT_PREFIX)/client.o
			
clean:
			-rm -f $(TEST_PREFIX) *.o test_stress test_error_codes
			-rm -f $(EXAMPLE_PREFIX) *.o example_client
			-rm -f $(CLIENT_PREFIX) *.o
			cd $(DRIVER_PREFIX)/ && make clean 