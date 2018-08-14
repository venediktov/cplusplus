const bs58 = require('bs58')
const EthCrypto = require('eth-crypto');
const ecc = require('eosjs-ecc');
const eth = require('ethereumjs-util');
var keyUtils = require('eosjs-ecc/lib//key_utils');

const identity = EthCrypto.createIdentity();
public_address_hex_str = identity.address; //do not remove 0x
private_key_hex_str = identity.privateKey // do not remove 0x
public_key_hex_str = identity.publicKey;
// private_key_hex_str = '95040f2b28c185eb630d61665369b3a70e2c2d2819d84aa58998e4a4de9e5899'
console.log(identity);
console.log("######################################  ETH");
const message = 'EOS can handle ETH signatures';
const messageHash = EthCrypto.hash.keccak256(message);
console.log("messageHash:");
console.log(messageHash);
const signature = EthCrypto.sign(
    private_key_hex_str, // privateKey
    messageHash // hash of message
);
console.log('signature:');
console.log(signature);
const signer = EthCrypto.recoverPublicKey(
    signature, //'0x388862e1a8e03c0d5012ca201c65c0eaecbe13f7418444e8788cd2f358bf254214d413e7e033d08629cd3a50ed413e947d289e2bedf34a133048b39fa509d7cc1c', // signature
    messageHash //'0xf994cb4eec41aac621f0618bdd498c0c529dcf7c86e31e66d953f4b6d4299d15' // message hash
);
console.log('signer:');
console.log(signer);
console.log("######################################   EOS");

// ecc.randomKey().then(privateKey => {
// console.log('Private Key:\t', privateKey) // wif
// console.log('Public Key:\t', ecc.privateToPublic(privateKey)) // EOSkey...
// console.log('Private Key HEX:\t', bs58.decode(privateKey).toString('hex')) // base58 nochecksum   HEX...
// console.log('Private Key HEX length:\t', bs58.decode(privateKey).toString('hex').length) // EOSkey HEX...
// console.log('Private Key HEX checked:\t', keyUtils.checkDecode(privateKey, 'sha256x2').toString('hex')) // EOSkey HEX...
// console.log('Private Key HEX checked length:\t', keyUtils.checkDecode(privateKey, 'sha256x2').toString('hex').length)
// console.log('back to Private key:\t' , keyUtils.checkEncode(keyUtils.checkDecode(privateKey, 'sha256x2'), 'sha256x2'))
// });


console.log('private_key_hex_str:');
console.log(private_key_hex_str);

private_key = Buffer.from(private_key_hex_str.slice(2),'hex');
private_key= Buffer.concat([new Buffer([0x80]), private_key]);
eos_private_key_wif = keyUtils.checkEncode(private_key, 'sha256x2');

console.log("eos_private_key_base58:");
console.log(eos_private_key_wif);
console.log(ecc.isValidPrivate(eos_private_key_wif) === true);

console.log("message:")
console.log(message);
console.log("messageHash:")
console.log(messageHash)

const eos_signature = ecc.sign(message, eos_private_key_wif, 'hex');
const eos_signed_hash = ecc.signHash(messageHash.slice(2), eos_private_key_wif, 'hex');

console.log("eos_signature for plain message:");
console.log(eos_signature);
console.log("eos_signature for hash message:");
console.log(eos_signed_hash);

console.log("PUBLIC KEY RECOVERY:");
const ms = Buffer.from( messageHash, 'hex');
const e = ecc.recover(eos_signature, ms);
console.log(e);

console.log("Exeute following chain command:")
cleosCmd = "cleos push action sense.snap keycheck '[" + messageHash.slice(2) + " , " +  eos_signed_hash + "]' -p sense.snap";
console.log(cleosCmd);
