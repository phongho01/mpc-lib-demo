const { ethers } = require('ethers');
const fs = require('fs');

// network variables
const RPC_PROVIDER = 'https://rpc-mumbai.maticvigil.com';
const provider = new ethers.providers.JsonRpcProvider(RPC_PROVIDER);

const generateUnsignedTransaction = async () => {
  try {

    const nonce = await provider.getTransactionCount('0xCa7539fF485cbF44cd4238433206c4FD29876155');
    const unsignedTx = {
      to: '0x4F9EF07A6DDF73494D2fF51A8f7B78e9c5815eb2',
      value: ethers.utils.parseEther('0.00000001'),
      type: 2,
      maxFeePerGas: '0x59682f1e',
      maxPriorityFeePerGas: '0x59682f00',
      nonce,
      gasLimit: '0x5208',
      chainId: 80001
    }

    const serializedUnsignedTx = ethers.utils.serializeTransaction(unsignedTx);
    const txHash = ethers.utils.keccak256(serializedUnsignedTx);
    console.log('Unsigned transaction hash: ', txHash);
    fs.writeFileSync(__dirname + '/messageHash.txt', txHash.slice(2))
    fs.writeFileSync(__dirname + '/unsignedTx.json', JSON.stringify(unsignedTx))
  } catch (error) {
    console.error('error', error);
  }
}

generateUnsignedTransaction();