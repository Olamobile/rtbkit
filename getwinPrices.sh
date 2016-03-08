#!/bin/bash

echo "Number of bid requests:"
cat AUCTION*.log | wc -l 
echo "Number of bids:"
cat BID*log | wc -l 

echo "Number of won auctions:"
cat ola_campaign_1/fixed/MATCHED*.log | wc -l 
echo "Sum of all bid prices:"
cat BID*.log  | awk  'BEGIN { FS = "," } ; { print $3 }'| awk  'BEGIN { FS = "\"" } ; { print $4 }' |  sed 's/USD\/1M//' | awk '{ sum += $1 } END { print sum/1000 }'
echo "Sum of all win prices:"
cat ola_campaign_1/fixed/MATCHED*.log  | awk  'BEGIN { FS = " " } ; { print $8 }' | sed 's/USD\/1M//' | awk '{ sum += $1 } END { print sum/1000 }'
