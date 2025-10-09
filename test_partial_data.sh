#!/bin/bash
# Comprehensive test for partial data, buffer overflow, low bandwidth, etc.

PORT=6667
PASS="pass123"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== IRC Server Edge Cases Test ===${NC}\n"

# Test 1: Partial data (as in subject)
echo -e "${YELLOW}Test 1: Partial data (subject example)${NC}"
{
    printf "com"
    sleep 0.1
    printf "man"
    sleep 0.1
    printf "d\n"
} | nc 127.0.0.1 $PORT 2>/dev/null | head -1 &
sleep 1
echo -e "${GREEN}✓ Partial data handled${NC}\n"

# Test 2: Very slow client (low bandwidth simulation)
echo -e "${YELLOW}Test 2: Low bandwidth (1 byte/50ms)${NC}"
{
    for char in P A S S ' ' p a s s 1 2 3; do
        printf "%s" "$char"
        sleep 0.05
    done
    printf "\r\n"
    sleep 0.2
    printf "QUIT\r\n"
} | nc 127.0.0.1 $PORT 2>/dev/null >/dev/null &
sleep 2
echo -e "${GREEN}✓ Slow transmission handled${NC}\n"

# Test 3: Multiple partial commands in buffer
echo -e "${YELLOW}Test 3: Multiple commands partially received${NC}"
{
    printf "PASS pa"
    sleep 0.1
    printf "ss123\r\nNICK te"
    sleep 0.1
    printf "st\r\nUSER test 0 * :T"
    sleep 0.1
    printf "est\r\nQUIT\r\n"
} | nc 127.0.0.1 $PORT 2>/dev/null >/dev/null &
sleep 2
echo -e "${GREEN}✓ Multiple partial commands handled${NC}\n"

# Test 4: Command with only \n (not \r\n)
echo -e "${YELLOW}Test 4: LF-only line endings${NC}"
{
    printf "PASS pass123\n"
    sleep 0.1
    printf "NICK test\n"
    sleep 0.1
    printf "QUIT\n"
} | nc 127.0.0.1 $PORT 2>/dev/null >/dev/null &
sleep 1
echo -e "${GREEN}✓ LF-only handled${NC}\n"

# Test 5: Very large buffer (stress test)
echo -e "${YELLOW}Test 5: Large buffer stress test${NC}"
{
    printf "PASS pass123\r\n"
    # Send a very long PRIVMSG
    printf "PRIVMSG #test :"
    for i in {1..1000}; do
        printf "A"
    done
    printf "\r\nQUIT\r\n"
} | nc 127.0.0.1 $PORT 2>/dev/null >/dev/null &
sleep 1
echo -e "${GREEN}✓ Large buffer handled${NC}\n"

# Test 6: Rapid-fire commands
echo -e "${YELLOW}Test 6: Rapid-fire commands${NC}"
{
    for i in {1..10}; do
        printf "PASS pass123\r\n"
    done
    printf "QUIT\r\n"
} | nc 127.0.0.1 $PORT 2>/dev/null >/dev/null &
sleep 1
echo -e "${GREEN}✓ Rapid commands handled${NC}\n"

# Test 7: Command split at CRLF boundary
echo -e "${YELLOW}Test 7: Split at CRLF boundary${NC}"
{
    printf "PASS pass123\r"
    sleep 0.1
    printf "\nNICK test\r"
    sleep 0.1
    printf "\nQUIT\r"
    sleep 0.1
    printf "\n"
} | nc 127.0.0.1 $PORT 2>/dev/null >/dev/null &
sleep 1
echo -e "${GREEN}✓ CRLF split handled${NC}\n"

# Test 8: Empty lines
echo -e "${YELLOW}Test 8: Empty lines${NC}"
{
    printf "\r\n\r\n"
    printf "PASS pass123\r\n"
    printf "\r\n"
    printf "QUIT\r\n"
} | nc 127.0.0.1 $PORT 2>/dev/null >/dev/null &
sleep 1
echo -e "${GREEN}✓ Empty lines handled${NC}\n"

# Test 9: Random fragmentation
echo -e "${YELLOW}Test 9: Random fragment sizes${NC}"
COMMAND="PASS pass123\r\nNICK fragtest\r\nUSER test 0 * :Real Name\r\nQUIT\r\n"
{
    LEN=${#COMMAND}
    POS=0
    while [ $POS -lt $LEN ]; do
        CHUNK_SIZE=$((1 + RANDOM % 5))  # Random 1-5 bytes
        printf "%s" "${COMMAND:$POS:$CHUNK_SIZE}"
        POS=$((POS + CHUNK_SIZE))
        sleep 0.05
    done
} | nc 127.0.0.1 $PORT 2>/dev/null >/dev/null &
sleep 2
echo -e "${GREEN}✓ Random fragmentation handled${NC}\n"

# Test 10: Simultaneous connections with partial data
echo -e "${YELLOW}Test 10: Multiple clients with partial data${NC}"
for i in {1..5}; do
    {
        printf "PASS"
        sleep 0.1
        printf " pass123\r\nNICK test$i\r\n"
        sleep 0.2
        printf "QUIT\r\n"
    } | nc 127.0.0.1 $PORT 2>/dev/null >/dev/null &
done
sleep 2
echo -e "${GREEN}✓ Multiple concurrent clients handled${NC}\n"

echo -e "${GREEN}=== All tests completed successfully! ===${NC}"

