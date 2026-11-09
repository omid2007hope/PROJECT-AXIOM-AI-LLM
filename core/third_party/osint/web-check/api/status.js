import http from 'http';
import https from 'https';
import middleware from './_common/middleware.js';
import { UA } from './_common/http.js';

// How long to wait for a response before giving up, in milliseconds
const REQUEST_TIMEOUT = parseInt(process.env.PUBLIC_API_TIMEOUT_LIMIT || '40000', 10);

// Request a page on a fresh connection, noting how many ms in each step ended
const timeRequest = (url) =>
  new Promise((resolve, reject) => {
    const start = performance.now();
    const marks = {};
    const mark = (step) => (marks[step] = Math.round(performance.now() - start));
    const client = url.startsWith('http:') ? http : https;
    const options = { agent: false, headers: { 'user-agent': UA } };

    const req = client.get(url, options, (res) => {
      mark('firstByte');
      res.resume();
      res.on('error', fail);
      res.on('end', () => {
        mark('done');
        clearTimeout(killTimer);
        resolve({ status: res.statusCode, marks });
      });
    });
    req.on('socket', (socket) => {
      socket.once('lookup', () => mark('dns'));
      socket.once('connect', () => mark('connect'));
      socket.once('secureConnect', () => mark('tls'));
    });
    req.on('error', fail);

    function fail(error) {
      clearTimeout(killTimer);
      reject(error);
    }

    // Abort the request and close its socket once the deadline passes
    const killTimer = setTimeout(() => {
      req.destroy(new Error(`Request timed-out after ${REQUEST_TIMEOUT} ms`));
    }, REQUEST_TIMEOUT);
    killTimer.unref?.();
  });

const statusHandler = async (url) => {
  const { status, marks } = await timeRequest(url);
  if (status < 200 || status >= 400) {
    throw new Error(`Received non-success response code: ${status}`);
  }
  const { dns = 0, connect, tls = connect, firstByte, done } = marks;
  return {
    isUp: true,
    responseCode: status,
    responseTime: done,
    timings: {
      dns,
      connect: connect - dns,
      tls: tls - connect,
      wait: firstByte - tls,
      download: done - firstByte,
    },
  };
};

export const handler = middleware(statusHandler);
export default handler;
