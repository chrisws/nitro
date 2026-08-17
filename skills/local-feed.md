# Local Feed

Use this when the goal is a personal, low-noise "what's happening locally" page — the kind of
thing someone builds after deleting Facebook but still wants the actual useful signal it
carried: police/safety alerts, local community groups, local sport, a hobby community (birding,
markets, whatever), minus the opinions, ads, and algorithm. Pairs with `json-apis` for the
fetch discipline and `vanilla-web` for the rendering — this skill is about *sourcing*: finding
real feeds for things that mostly don't have documented APIs.

## Step 1: scope it with the user

Local info needs are specific to a person and a place — don't guess categories. Get concrete on:

- **Location** — suburb/region/state, since "local" only means something with a place attached.
- **Categories** — police/safety alerts, specific sports and specific teams (not "sport" in
  general), specific interest communities (birding, gardening, a hobby group), council/civic
  notices. A vague "local news" ask produces a generic page nobody actually wants to check daily.
- **What to explicitly exclude** — this is often the actual point (per the motivating case:
  sick of the opinions and arguments, wants the factual updates only). If the user says this,
  treat "no comments/discussion threads, headlines and official releases only" as a hard
  filter, not a nice-to-have.

## Step 2: find real sources for each category

Work through this priority order per category, and don't fabricate a feed that doesn't exist:

1. **A documented keyless JSON API**, if one exists for that category. Sport is the best case —
   e.g. Squiggle (`api.squiggle.com.au`) for AFL: keyless, query-param based
   (`?q=games`, `?q=standings`, `?q=teams`, add `;team=<name>` to filter), JSON by default. Per
   its own usage terms: set a descriptive `User-Agent` with a contact email, cache results
   rather than re-polling, and don't request more than you need (a single team, not the whole
   season, if that's all that's wanted).
2. **The organization's own RSS feed**, if no API exists. Check for it directly: try
   `/feed`, `/rss`, `/rss.xml` on the org's site, or fetch the site's homepage HTML and look for
   `<link rel="alternate" type="application/rss+xml" ...>` in the `<head>` — that's the standard
   autodiscovery tag and is more reliable than guessing a path. Government/police sites often
   don't publish one at all; check before assuming.
3. **A subreddit or similar community feed**, if the interest has an active one — Reddit's
   `.json` suffix on any listing URL works keylessly (with a descriptive `User-Agent`, same as
   any other API) and often surfaces the same "someone posted about it" signal a local Facebook
   group did, with less noise if you filter for link/announcement posts over discussion threads.
4. **Say so plainly if nothing usable exists.** Some categories (a small police force's news
   page, a niche hobby group with no online presence beyond a Facebook page) genuinely have no
   feed, RSS, or API — Facebook groups and pages in particular are usually a dead end
   programmatically, since third-party access to Facebook's Graph API requires app review and
   isn't reachable from a plain `TOOL:CURL`. Tell the user this rather than quietly working
   around it or inventing a feed shape.

## Step 3: filter for signal, not just fetch everything

- If a source mixes official releases with a comment/discussion layer (a subreddit, a public
  Facebook-alternative feed), keep the post/headline and drop the discussion thread — that's
  usually the exact noise being avoided.
- For police/safety-style feeds specifically, these are meant to be scanned quickly — headline
  and one-line summary per item, not the full release text, with a link/reference to the source
  for anyone who wants the detail.
- Don't editorialize or add commentary of your own to factual alerts (police releases, safety
  notices) — render them as reported, since altering tone on this category specifically is
  where trust in an aggregator breaks fastest.

## Step 4: assemble and keep it current

- Hand fetched, real data to `vanilla-web` for rendering — group by category, most-recent-first
  within each, and design each section to degrade gracefully if one source is temporarily down
  (see `json-apis` on graceful degradation) rather than blanking the whole page.
- Decide and state a refresh cadence — this kind of page goes stale fast and stale local-safety
  info is actively misleading, more so than a stale news headline. A checked-daily local feed
  needs a visible "last updated" timestamp at minimum, and a documented way to re-run/regenerate
  it if it's not live-fetching.

## What NOT to do

- Don't fabricate a feed URL for an organization that doesn't publish one — check, and report
  honestly if nothing exists rather than guessing a plausible-looking path.
- Don't attempt to scrape Facebook groups/pages as a source — it's the one platform in this
  space that's genuinely not reachable this way, and pretending otherwise wastes effort.
- Don't include discussion/comment noise from a source that mixes it with real signal —
  filtering that out is usually the entire reason this page is being built.
- Don't silently let the page go stale — a local-alerts page with no visible freshness
  indicator is worse than no page.
