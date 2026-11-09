import styled from '@emotion/styled';
import { checks, type CheckId } from '@/data/checks';
import colors from 'client/styles/colors';

const Docs = styled.aside`
  font-size: 0.9rem;
  line-height: 1.6;
  color: ${colors.textColorSecondary};
  p,
  ul {
    margin: 0 0 1.5rem;
  }
  ul {
    padding-left: 1rem;
    overflow-wrap: anywhere;
  }
  a {
    color: ${colors.primaryIncreasedContrast};
  }
`;

// What a check looks at, why it matters, and where to read more
const DocContent = ({ id }: { id: CheckId }) => {
  const { description, use, resources } = checks[id];
  return (
    <Docs>
      <h3>What it is</h3>
      <p>{description}</p>
      <h3>Why it matters</h3>
      <p>{use}</p>
      {resources.length > 0 && (
        <>
          <h3>Further reading</h3>
          <ul>
            {resources.map((resource) => {
              const { title, link } =
                typeof resource === 'string' ? { title: resource, link: resource } : resource;
              return (
                <li key={link}>
                  <a href={link} target="_blank" rel="noreferrer">
                    {title}
                  </a>
                </li>
              );
            })}
          </ul>
        </>
      )}
      <a href={`/${id}`} target="_blank" rel="noreferrer">
        More about this check →
      </a>
    </Docs>
  );
};

export default DocContent;
