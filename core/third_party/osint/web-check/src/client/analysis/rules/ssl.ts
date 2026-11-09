import type { Analyzer, Result } from '../types';
import { daysUntil } from '../helpers';
import { formatNumber } from 'client/utils/format';

// Plain reasons for the validation errors sites most often get
export const REASONS: Record<string, string> = {
  UNABLE_TO_VERIFY_LEAF_SIGNATURE: "Server doesn't send its intermediate certificate",
  UNABLE_TO_GET_ISSUER_CERT_LOCALLY: "Server doesn't send its intermediate certificate",
  DEPTH_ZERO_SELF_SIGNED_CERT: 'Self-signed, not issued by a trusted authority',
  SELF_SIGNED_CERT_IN_CHAIN: "Chain ends at a root browsers don't trust",
  ERR_TLS_CERT_ALTNAME_INVALID: "Certificate doesn't cover this hostname",
};

// A problem with when the certificate expires, or null when it's over a month away
export const checkExpiry = (validTo: unknown): Result | null => {
  const days = daysUntil(validTo);
  if (days === null || days > 30) return null;
  if (days < 0) {
    return {
      severity: 'critical',
      title: 'SSL certificate expired',
      detail: `Expired ${formatNumber(-days)} day(s) ago`,
    };
  }
  if (days <= 7) {
    return {
      severity: 'critical',
      title: 'SSL certificate expiring within a week',
      detail: `Expires in ${days} day(s), renew immediately`,
    };
  }
  if (days <= 14) {
    return {
      severity: 'issue',
      title: 'SSL certificate expiring soon',
      detail: `Expires in ${days} day(s), schedule renewal`,
    };
  }
  return {
    severity: 'warning',
    title: 'SSL certificate renews within a month',
    detail: `Expires in ${days} day(s)`,
  };
};

// Check certificate validity and expiry window
const ssl: Analyzer = (d) => {
  const out: ReturnType<Analyzer> = [];
  if (d.isValid === false) {
    out.push({
      severity: 'critical',
      title: 'SSL certificate invalid',
      detail: REASONS[d.authError] || d.authError || 'Certificate failed validation',
    });
  } else if (d.isValid === true) {
    out.push({ severity: 'pass', title: 'SSL certificate valid' });
  }
  const expiry = checkExpiry(d.valid_to);
  if (expiry) out.push(expiry);
  return out;
};

export default ssl;
