#!/bin/bash

echo "Testing IRC server on localhost:6667"
echo "====================================="

# Test CAP LS
echo -e "Testing CAP LS..."
echo -e "CAP LS\r\n" | nc localhost 6667

echo -e "\nTesting PASS..."
echo -e "PASS testpass\r\n" | nc localhost 6667

echo -e "\nTesting NICK..."
echo -e "NICK jsommet\r\n" | nc localhost 6667

echo -e "\nTesting USER..."
echo -e "USER jsommet jsommet localhost :Joseph Sommet\r\n" | nc localhost 6667

echo -e "\nTests completed."
