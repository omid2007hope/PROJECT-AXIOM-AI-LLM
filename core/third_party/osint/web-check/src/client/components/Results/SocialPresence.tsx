import { faXTwitter, faBluesky, faGithub, faMastodon } from '@fortawesome/free-brands-svg-icons';
import type { IconDefinition } from '@fortawesome/fontawesome-svg-core';
import { Card } from 'client/components/Form/Card';
import Row, { Section, type RowProps } from 'client/components/Form/Row';
import Icon from 'client/components/misc/Icon';
import { formatNumber } from 'client/utils/format';

const cardStyles = `
  h4 {
    gap: 0.5rem;
  }
  img.avatar {
    width: 1.2rem;
    height: 1.2rem;
    border-radius: 50%;
    vertical-align: text-bottom;
    margin-right: 0.35rem;
  }
`;

const ICONS: Record<string, IconDefinition> = {
  X: faXTwitter,
  Bluesky: faBluesky,
  GitHub: faGithub,
  Mastodon: faMastodon,
};

const LABELS: Record<string, string> = { X: 'X (Twitter)' };

const verification = (profile: any): RowProps => {
  if (profile.checked === false) {
    return {
      val: 'Unknown',
      status: 'info',
      reason: 'The network could not be reached, so this account was not checked',
    };
  }
  if (!profile.exists) {
    return { val: false, reason: 'This handle does not resolve to a live account' };
  }
  if (profile.domainVerified) {
    return {
      val: true,
      reason: 'The handle is this domain, so the network has verified control of it',
    };
  }
  if (profile.linksBack) {
    return {
      val: true,
      reason: profile.websiteVerified
        ? 'The profile links back to this site, and the network has verified the link'
        : 'The profile links back to this site',
    };
  }
  return {
    val: false,
    status: 'warning',
    reason: profile.website
      ? `The profile links to ${profile.website} instead`
      : 'The profile lists no website to check against',
  };
};

const num = (value: any): string =>
  typeof value === 'number' ? formatNumber(value) : String(value);

const SocialPresenceCard = (props: {
  data: any;
  title: string;
  actionButtons: any;
}): JSX.Element => {
  const profiles: any[] = props.data || [];
  return (
    <Card heading={props.title} actionButtons={props.actionButtons} styles={cardStyles}>
      {profiles.map((profile) => (
        <Section
          key={`${profile.network}-${profile.handle}`}
          title={
            <>
              {ICONS[profile.network] && <Icon icon={ICONS[profile.network]} />}
              {LABELS[profile.network] || profile.network}
            </>
          }
        >
          <Row>
            <span className="lbl">Handle</span>
            <span className="val">
              <a target="_blank" rel="noreferrer" href={profile.url}>
                {profile.avatar && <img className="avatar" src={profile.avatar} alt="" />}@
                {profile.handle}
              </a>
            </span>
          </Row>
          {profile.name && <Row lbl="Name" val={profile.name} />}
          {profile.joined && <Row lbl="Joined" val={profile.joined} />}
          {profile.count !== undefined && (
            <Row
              lbl={profile.network === 'GitHub' ? 'Repositories' : 'Posts'}
              val={num(profile.count)}
            />
          )}
          {profile.followers !== undefined && <Row lbl="Followers" val={num(profile.followers)} />}
          {profile.website && !profile.linksBack && <Row lbl="Website" val={profile.website} />}
          <Row lbl="Verification" term="social-verification" {...verification(profile)} />
        </Section>
      ))}
    </Card>
  );
};

export default SocialPresenceCard;
