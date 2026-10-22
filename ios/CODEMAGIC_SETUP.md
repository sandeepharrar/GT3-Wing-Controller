# Getting GT3 Wing Controller onto your iPhone with Codemagic and TestFlight

Everything here works from Windows in a web browser. You never need a Mac.
Never paste keys, passwords or `.p8` files into chat. Upload them only to the Apple and Codemagic websites.

Before you start, check one thing: the project targets **iOS 26.2**, so your iPhone must be on iOS 26.2 or newer (Settings > General > About).

## 1. Get the code onto `main` (about 1 min)

The app, `codemagic.yaml` and this guide are on the `codemagic-setup` branch of `sandeepharrar/GT3-Wing-Controller`. Merge that pull request on GitHub so they're on `main`, because Codemagic builds from `main`.

## 2. Register the app with Apple (about 10 min)

1. Go to https://developer.apple.com/account > **Certificates, Identifiers & Profiles** > **Identifiers** > **+**.
2. Choose **App IDs** > **App**. Description: `GT3 Wing Controller`. Bundle ID: **Explicit**, `com.sandeep.GT3-Wing-Controller`. Leave the capabilities alone and click **Register**.
   - If Apple says the ID is taken, it is probably registered under another team. Tell Claude and we'll pick a new one (the project and `codemagic.yaml` both need updating).
3. Go to https://appstoreconnect.apple.com > **Apps** > **+** > **New App**.
   - Platform: iOS. Name: `GT3 Wing Controller` (if that name is taken, add something, like `GT3 Wing Controller Z4`; it only has to be unique on the App Store, and nobody will see it).
   - Language: English. Bundle ID: pick the one you just made. SKU: `gt3wing`. Access: Full Access. Click **Create**.

## 3. Create an App Store Connect API key (about 3 min)

This lets Codemagic sign and upload the app on your behalf.

1. In App Store Connect go to **Users and Access** > **Integrations** > **App Store Connect API** > **Team Keys**. If asked, click **Request Access** first and accept.
2. Click **+** (Generate API Key). Name: `Codemagic`. Access: **App Manager**. Click **Generate**.
3. Note the **Issuer ID** (top of the page) and the key's **Key ID**.
4. Click **Download API Key** to save the `.p8` file. Apple only lets you download it **once**, so keep it somewhere safe.

## 4. Connect Codemagic (about 10 min)

1. Sign up at https://codemagic.io with your GitHub account (the free tier gives 500 Mac build minutes a month; one build takes roughly 5 to 10).
2. **Add the API key:** Team settings (or your personal account settings) > **Team integrations** > **Developer Portal** > **Connect** / **Manage keys** > **Add key**.
   - Name it exactly `GT3 Wing ASC Key` (codemagic.yaml looks it up by that name).
   - Enter the Issuer ID and Key ID, and upload the `.p8` file.
3. **Make a distribution certificate:** settings > **codemagic.yaml settings** > **Code signing identities** > **iOS certificates** > **Generate certificate**.
   - Type: **Apple Distribution**. API key: `GT3 Wing ASC Key`. Reference name: `gt3_dist`.
   - Codemagic offers the `.p12` and its password for download **once**. Save both somewhere safe.
4. **Make the provisioning profile** (back on Apple's site): https://developer.apple.com/account > **Profiles** > **+** > **App Store Connect** (under Distribution) > pick the `com.sandeep.GT3-Wing-Controller` App ID > pick the certificate Codemagic just made > name it `GT3 Wing App Store` > **Generate**.
5. **Pull that profile into Codemagic:** Code signing identities > **iOS provisioning profiles** > **Fetch profiles**, choose `GT3 Wing ASC Key`, tick `GT3 Wing App Store`, give it the reference name `gt3_appstore`, and **Download selected**.
6. **Add the app:** Codemagic dashboard > **Add application** > GitHub > pick `GT3-Wing-Controller` > project type **iOS App**. Codemagic finds `codemagic.yaml` automatically.

## 5. First build

1. On the app's page in Codemagic, click **Start new build**, pick branch `main` and workflow **iOS TestFlight**.
2. When it finishes (green), the build appears in App Store Connect > your app > **TestFlight** after 5 to 30 minutes of Apple processing.
3. If App Store Connect asks about **export compliance / encryption**, answer that the app uses **no encryption** (or only exempt encryption). The app only uses Bluetooth, so this is the honest answer.
4. In the TestFlight tab, under **Internal Testing**, click **+** to create a group (for example `Me`), and add yourself (your Apple ID is already a team member).
5. On your iPhone, install the **TestFlight** app from the App Store, sign in with the same Apple ID, and install GT3 Wing Controller.

After that, every push to `main` that changes something in `ios/` builds and uploads a new version automatically, and TestFlight updates it on your phone. Changes that only touch `firmware/` don't start a build, so they don't use up build minutes.

## Good to know

- A TestFlight build works for **90 days**. Before it runs out, start a new build (or push any change).
- When your Apple Developer membership lapses, you can't upload new builds, and the installed build stops working when its 90 days are up.
- Build failed? Open the build in Codemagic, download the log under **Artifacts**, and share it with Claude (the log has no secrets in it).
