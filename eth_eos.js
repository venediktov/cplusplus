const bs58 = require('bs58')
const EthCrypto = require('eth-crypto');
const ecc = require('eosjs-ecc');
const eth = require('ethereumjs-util');
var keyUtils = require('eosjs-ecc/lib//key_utils');

const identity = EthCrypto.createIdentity();
public_address_hex_str = identity.address; //do not remove 0x
private_key_hex_str = identity.privateKey // do not remove 0x
public_key_hex_str = identity.publicKey;
//private_key_hex_str = '0xabb5fccd075a18a480636665267acbdeaa5a81e394258fa95d77eba8aeeb20fa' //bad
// private_key_hex_str = '0x95040f2b28c185eb630d61665369b3a70e2c2d2819d84aa58998e4a4de9e5899' //good
console.log(identity);
console.log("######################################  ETH");
const message = 'EOS can handle ETH signatures';
const messageHash = EthCrypto.hash.keccak256(message);
console.log("messageHash:");
console.log(messageHash);
const eth_signature = EthCrypto.sign(
    private_key_hex_str, // privateKey
    messageHash // hash of message
);
console.log('eth_signature:');
console.log(eth_signature);
const eth_signer = EthCrypto.recoverPublicKey(
    eth_signature, //'0x388862e1a8e03c0d5012ca201c65c0eaecbe13f7418444e8788cd2f358bf254214d413e7e033d08629cd3a50ed413e947d289e2bedf34a133048b39fa509d7cc1c', // signature
    messageHash //'0xf994cb4eec41aac621f0618bdd498c0c529dcf7c86e31e66d953f4b6d4299d15' // message hash
);
console.log("PUBLIC KEY RECOVERY ( ETH Library) :");
console.log(eth_signer.slice(0,64) + " + " + eth_signer.slice(64));
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

console.log("eos_private_key_wif:");
console.log(eos_private_key_wif);
console.log(ecc.isValidPrivate(eos_private_key_wif) === true);

console.log("message:")
console.log(message);
console.log("messageHash:")
console.log(messageHash)

const eos_signature = ecc.sign(message, eos_private_key_wif);
const eos_signed_hash = ecc.signHash(messageHash.slice(2), eos_private_key_wif, 'hex');

console.log("eos_signature for plain message:");
console.log(eos_signature);
console.log("eos_signature for hash message:");
console.log(eos_signed_hash);

console.log("PUBLIC KEY RECOVERY ( EOS Library ) :");
const ms = Buffer.from( messageHash.slice(2), 'hex');
// const e = ecc.recover(eos_signature, ms);
const e = ecc.recoverHash(eos_signed_hash, ms);
const prefix = keyUtils.checkDecode(e.slice(3)).toString('hex').slice(0,2);
const key = keyUtils.checkDecode(e.slice(3)).toString('hex').slice(2);
console.log(e + " ===> " + prefix + " + " + key );

console.log("Execute following chain command:")
cleosCmd = "cleos push action sense.snap keycheck '[" + messageHash.slice(2) + " , " +  eos_signed_hash + "]' -p sense.snap";
console.log(cleosCmd);

console.log("EOS SIG IN HEX:")
eos_signed_hash_hex = keyUtils.checkDecode(eos_signed_hash.slice(7), 'K1').toString('hex');
console.log(eos_signed_hash_hex.slice(0,2) + " + " + eos_signed_hash_hex.slice(2));

console.log("ETH SIG IN HEX:")
eth_signed_hash_hex = eth_signature.slice(2);
console.log(eth_signed_hash_hex.slice(0,-2) + " + " + eth_signed_hash_hex.slice(-2));
