import tls from 'tls';
import middleware from './_common/middleware.js';
import { parseTarget } from './_common/parse-target.js';

const HANDSHAKE_TIMEOUT = 4000;

const VERSIONS = ['TLSv1', 'TLSv1.1', 'TLSv1.2', 'TLSv1.3'];

const isIp = (h) => /^[\d.]+$/.test(h) || h.includes(':');

// Where to connect, with SNI for hostnames
const connectTo = ({ hostname, port }) => ({
  host: hostname,
  port: port ? Number(port) : 443,
  ...(isIp(hostname) ? {} : { servername: hostname }),
});

// Open one TLS handshake to the host and capture what was negotiated
const handshake = (target) =>
  new Promise((resolve, reject) => {
    let ocspStapled = false;
    const socket = tls.connect({
      ...connectTo(target),
      ALPNProtocols: ['h2', 'http/1.1'],
      rejectUnauthorized: false,
      requestOCSP: true,
    });
    socket.on('OCSPResponse', (res) => {
      ocspStapled = res?.length > 0;
    });

    let settled = false;
    const finish = (fn) => (arg) => {
      if (settled) return;
      settled = true;
      fn(arg);
    };
    socket.setTimeout(HANDSHAKE_TIMEOUT);
    socket.once(
      'secureConnect',
      finish(() => {
        const cipher = socket.getCipher();
        const protocol = socket.getProtocol();
        const ephemeral =
          typeof socket.getEphemeralKeyInfo === 'function' ? socket.getEphemeralKeyInfo() : null;
        const result = {
          protocol,
          cipher: cipher && {
            name: cipher.name,
            standardName: cipher.standardName,
            version: cipher.version,
          },
          alpnProtocol: socket.alpnProtocol || null,
          sessionResumption: !!socket.getSession(),
          forwardSecrecy: protocol === 'TLSv1.3' || (!!cipher && /^(ECDHE|DHE)/.test(cipher.name)),
          authorized: socket.authorized,
          authError: socket.authorizationError ? String(socket.authorizationError) : null,
          ephemeralKey: ephemeral && {
            type: ephemeral.type || null,
            name: ephemeral.name || null,
            size: ephemeral.size || null,
          },
          ocspStapled,
        };
        socket.end();
        resolve(result);
      }),
    );
    socket.once(
      'timeout',
      finish(() => {
        socket.destroy();
        reject(new Error('TLS handshake timed-out'));
      }),
    );
    socket.once(
      'error',
      finish((err) => {
        socket.destroy();
        reject(new Error(`TLS handshake failed: ${err.reason || err.message}`));
      }),
    );
  });

// True for errors from the server turning a TLS version down, not from the network failing
const isRefusal = ({ code = '' }) =>
  code === 'ECONNRESET' ||
  (code.startsWith('ERR_SSL_') && code !== 'ERR_SSL_NO_PROTOCOLS_AVAILABLE');

// True if the server completes a handshake on one TLS version, false if refused, null if unknown
const accepts = (target, version) =>
  new Promise((resolve) => {
    const socket = tls.connect({
      ...connectTo(target),
      minVersion: version,
      maxVersion: version,
      ciphers: 'DEFAULT@SECLEVEL=0',
      rejectUnauthorized: false,
    });
    socket.setTimeout(HANDSHAKE_TIMEOUT, () => {
      resolve(null);
      socket.destroy();
    });
    socket.once('secureConnect', () => {
      resolve(true);
      socket.destroy();
    });
    socket.on('error', (e) => resolve(isRefusal(e) ? false : null));
    socket.once('close', () => resolve(false));
  });

const tlsConnectionHandler = async (url) => {
  const target = parseTarget(url);
  const [result, accepted] = await Promise.all([
    handshake(target),
    Promise.all(VERSIONS.map((version) => accepts(target, version))),
  ]);
  if (accepted.includes(null)) return result;
  const versions = VERSIONS.filter((v, i) => accepted[i] || v === result.protocol);
  return { ...result, versions };
};

export const handler = middleware(tlsConnectionHandler);
export default handler;
