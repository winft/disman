# Changelog
All notable changes to Disman will be documented in this file.
## [0.520.0-beta.1](https://gitlab.com/kwinft/disman/compare/disman@0.520.0-beta.0...disman@0.520.0-beta.1) (2020-10-02)


### Bug Fixes

* write primary property to config control file ([43d800c](https://gitlab.com/kwinft/disman/commit/43d800c1bd46cec076092f29fa22d2fc981d15db))
* **wayland:** add return to transform getters ([f6622f4](https://gitlab.com/kwinft/disman/commit/f6622f4d025fa9e8929b3bd143b12b590dbb0685))
* **wayland:** check null mode in wlroots backend ([e858f9a](https://gitlab.com/kwinft/disman/commit/e858f9a11ebc2d87ea55a8d6750b0d7267efe1de))
* **wayland:** omit sending any other data on disabling wlr heads ([61a06a8](https://gitlab.com/kwinft/disman/commit/61a06a89530c42224b2f03126c65872168cd082a))
* **wayland:** retry on cancelled wlroots config ([f94b33a](https://gitlab.com/kwinft/disman/commit/f94b33a1c293fedfc81e82679d571c9ff4876ce5))

## [0.520.0-beta.0](https://gitlab.com/kwinft/disman/compare/disman@0.519.0-beta.0...disman@0.520.0-beta.0) (2020-09-25)


### ⚠ BREAKING CHANGES

* **lib:** Refresh rates of modes are specified as integers in mHz.
* Edid class removed from public library.
* **lib:** AbstractBackend class renamed to Backend.
* **lib:** OutputList, ModeList renamed to OutputMap, ModeMap.
* **lib:** Basic types use std::shared_ptr instead of QSharedPointer.
* **lib:** API function names change.
* **lib:** Mode API changes.
* The control utility is renamed from `disman-doctor` to
`dismanctl`.
* **lib:** Output name API changes.
* **lib:** Disman::Edid API changes.
* **wayland:** The Disman Wayland library CMake target has been namespaced to
Disman::Wayland.
* **lib:** The Disman library CMake target has been namespaced to
Disman::Disman.
* Disman library include file names change.
* DISMAN_BACKEND_INPROCESS environement variable name changes to
DISMAN_IN_PROCESS.
* The Output API changes.

### Features

* compare with current config if change necessary ([49a096d](https://gitlab.com/kwinft/disman/commit/49a096da049049d1c0e57eaab789185c0fb427b0))
* **api:** add config generator ([eb07cbd](https://gitlab.com/kwinft/disman/commit/eb07cbdd35fdf0e158d2128706593e362626bfca))
* **api:** add config origin hint ([226b10d](https://gitlab.com/kwinft/disman/commit/226b10d3d5236bceb3e4b40f958b27f39900eda7))
* **api:** add replication source pointer getter ([2a1bc41](https://gitlab.com/kwinft/disman/commit/2a1bc41d5f928716ea19ad2961135ea4b9126581))
* **api:** condens individual output changed signals ([456a390](https://gitlab.com/kwinft/disman/commit/456a390abca3d7b6c97707786969fa1449059e39))
* **api:** improve Output debug log ([46d41fb](https://gitlab.com/kwinft/disman/commit/46d41fbe7b36d3224b6ab2e58d9a26c7646a1f7d))
* **api:** remove enforced mode size getter ([e4cc4ca](https://gitlab.com/kwinft/disman/commit/e4cc4cabc6e12f92d4d6c2afadc184f072f0d780))
* **api:** replace deprecated output hash function ([1a5d99b](https://gitlab.com/kwinft/disman/commit/1a5d99bffaa9435c9988db3b041e9c480b3ca412))
* **ctl:** provide watcher ([ca9a485](https://gitlab.com/kwinft/disman/commit/ca9a48562d633a9388cb7bfb0ee59cf65984c99e))
* **ctl:** remove DPMS functionality ([9c77e08](https://gitlab.com/kwinft/disman/commit/9c77e08d71f442061da66a6e1618c6463cfc8f57))
* **ctl:** show help and quit when launched without parameters ([aceb9ae](https://gitlab.com/kwinft/disman/commit/aceb9ae2b51e4a2bea9228e227440fae7cd2fe72))
* **lib:** add comparision functions for basic types ([3de94a3](https://gitlab.com/kwinft/disman/commit/3de94a318465ed08d922e20ab5fd13de5294618c))
* **lib:** generate config with disabled embedded output ([9b0ffa7](https://gitlab.com/kwinft/disman/commit/9b0ffa7656262a515461c26c744f4e5ab7543766))
* **lib:** handle refresh rates in mHz ([9ca5076](https://gitlab.com/kwinft/disman/commit/9ca5076a06d7bf030995f9538949c36454cc34c9))
* **lib:** improve config debug print ([36b40bf](https://gitlab.com/kwinft/disman/commit/36b40bf646f9e4c905ca6ed649e6c8b6e89741cd))
* **lib:** print debug tablet state for config ([1cb2ad7](https://gitlab.com/kwinft/disman/commit/1cb2ad709c155a0bafd07a1589562491682dbdbb))
* allow to set config file suffix ([015a28a](https://gitlab.com/kwinft/disman/commit/015a28a0432bbf4a0996b9bf40b5e89566973e5f))
* reconfigure on laptop lid being closed ([9ef4b8f](https://gitlab.com/kwinft/disman/commit/9ef4b8f02c0c48e9f6bb568b99f4adac02835001))
* set global output data through filer ([69acd9b](https://gitlab.com/kwinft/disman/commit/69acd9bfa86fcd20ab68fd153e45f6f418412ca5))
* **lib:** add Output description field ([f7b1239](https://gitlab.com/kwinft/disman/commit/f7b1239a0a2bdb5356de9e1f001317c2f08ff136))
* **lib:** log when EDID received ([000b706](https://gitlab.com/kwinft/disman/commit/000b706d4d2865d053c73d79b46ddeb8f68a43b2))
* **lib:** make Edid class copyable ([ec41c0c](https://gitlab.com/kwinft/disman/commit/ec41c0cdc5bf41c0392064490c296281d1bd5ff6))
* **lib:** provide global output data setter ([5abe02a](https://gitlab.com/kwinft/disman/commit/5abe02accdc2337dd212552e8b7ee14530fd6130))
* **lib:** remove Edid handle in Output class ([bf08dab](https://gitlab.com/kwinft/disman/commit/bf08dab89a74a1faaee895511181c301bfbbbf80))
* **lib:** remove output icon ([722ced7](https://gitlab.com/kwinft/disman/commit/722ced76b637f546b0dab73e3d710ef07840f204))
* **randr:** send config on output change ([9711d8b](https://gitlab.com/kwinft/disman/commit/9711d8b3106bb309b065d5b09d2334420708ec78))
* **randr:** set output description from EDID ([de8565f](https://gitlab.com/kwinft/disman/commit/de8565f6fb35ee5dfdc5a174df5c78e20b04fa9b))
* **wayland:** add debug output to config applying ([fa46448](https://gitlab.com/kwinft/disman/commit/fa46448e3e0cbf3c08eb9e472a67ca86734bd612))
* **wayland:** debug with expanded categories in plugins ([60f4dfe](https://gitlab.com/kwinft/disman/commit/60f4dfedd865c8fb6f1365665849d5d8421ed6c4))
* **wayland:** send config on output change ([ef5f77c](https://gitlab.com/kwinft/disman/commit/ef5f77ca90e04405ba8757e683acf1e9a10dc91c))
* **wayland:** set output description ([f74c9b0](https://gitlab.com/kwinft/disman/commit/f74c9b01b1aeaece048eee3064d79c7861291fe9))
* **wayland:** support zwlr_output_manager_v1 version 2 ([ea0a65f](https://gitlab.com/kwinft/disman/commit/ea0a65f04c9b89b1edd03a81611ba91fe6a34eed))
* add optional derived config when generating one ([de02c69](https://gitlab.com/kwinft/disman/commit/de02c69fbd141f0aca273265181c952605249d8e))
* auto-scale fractionally on config generation ([45b5ebd](https://gitlab.com/kwinft/disman/commit/45b5ebd618c49a2534a8e49476cd4a86832df3b7))
* enable out-of-process backend mode by default ([d9813bc](https://gitlab.com/kwinft/disman/commit/d9813bcfe502e1baf691832da80f14de8d003f51))
* establish control file write ([293cb50](https://gitlab.com/kwinft/disman/commit/293cb50687187da7d9591602d15fb6ea89b951db))
* homogenize backend logging ([7cfcc6c](https://gitlab.com/kwinft/disman/commit/7cfcc6cf52d8da362ef72effd094af394c992b5a))
* redesign output modes API ([61ef8fb](https://gitlab.com/kwinft/disman/commit/61ef8fba455d03e57947edee6475cafcdeab202b))
* remove output cloning ([c6f2b15](https://gitlab.com/kwinft/disman/commit/c6f2b156b8acb4711cf7356094c736faed99fc81))
* rename in-process environment variable ([2d41842](https://gitlab.com/kwinft/disman/commit/2d4184203c8c7a030e40dcf403dee76e3c786ad5))
* save full config as control file ([e318694](https://gitlab.com/kwinft/disman/commit/e3186947b3ddb85ad3ce502d4dd1a59a28288df7))
* save output description to control files ([4a9adcd](https://gitlab.com/kwinft/disman/commit/4a9adcd033111a09e8eb884d92442a0a473d052b))
* set output description in fake backend ([526aa0f](https://gitlab.com/kwinft/disman/commit/526aa0f80d698e6cbb92f6129b2c7aa6530009d6))
* set output hash explicitly in backends ([c227dd8](https://gitlab.com/kwinft/disman/commit/c227dd85487bb92f5aadbd6d17812aa4e8d4770a))
* **qscreen:** set output description ([f39427a](https://gitlab.com/kwinft/disman/commit/f39427a5b6db28740b0fb7245e6176ba2bd66ef6))
* **randr:** only report connected outputs ([de84057](https://gitlab.com/kwinft/disman/commit/de840571befe146ba159e434ea31edac6163af3a))


### Bug Fixes

* **lib:** deserialize size with custom function ([1906f64](https://gitlab.com/kwinft/disman/commit/1906f642becf51dfbd52372d943755e1ee220eda))
* **lib:** serialize resolution and refresh values by auto mode ([a408b97](https://gitlab.com/kwinft/disman/commit/a408b97d7fefb20609bfa10ad8ea3907d70f080f))
* always emit config changed on frontend changed ([af50054](https://gitlab.com/kwinft/disman/commit/af50054526baac9adc1647a696ce5d7b3dac7a25))
* auto-scale to 130 DPI instead of 110 DPI base value ([9a36174](https://gitlab.com/kwinft/disman/commit/9a361742724ddc5742f351e6e934aa2dc16fd5ed))
* compare mode refresh rate with epsilon ([3e2091a](https://gitlab.com/kwinft/disman/commit/3e2091afc61192d130f72f077439bafab6b58503))
* declare fetch_lid_closed as Q_SLOT ([1a649f2](https://gitlab.com/kwinft/disman/commit/1a649f21375f5a4578fda0dd6b6dbd07dbd591fe))
* do not try to load lid config with only one output ([9fb560a](https://gitlab.com/kwinft/disman/commit/9fb560ae52bc209fb636259938fc33a28c96075e))
* move open-lid file with static method and check that ([05f4c25](https://gitlab.com/kwinft/disman/commit/05f4c25020c988f91800f1b0cf74c97e9bf777f1))
* normalize positions on disabling embedded display ([5a74f5d](https://gitlab.com/kwinft/disman/commit/5a74f5d4ce9d377b27311464ea00a0851bda832e))
* on successful open lid file write go on ([466e2d5](https://gitlab.com/kwinft/disman/commit/466e2d5ce22c5cb69be615aef51d475a12c39e1d))
* stop timer on fast later lid closed change ([35ae62e](https://gitlab.com/kwinft/disman/commit/35ae62e3ed11eab7e7ceaed5368e8eeb0230a3d2))
* **api:** stream mode debug ([10e0926](https://gitlab.com/kwinft/disman/commit/10e09269f663576505532ab33aeb872495db835f))
* **ctl:** align info lines ([e9e1d91](https://gitlab.com/kwinft/disman/commit/e9e1d919851dd303932d6eb98d8ab630946ebff8))
* **ctl:** check for config error ([9afb0e3](https://gitlab.com/kwinft/disman/commit/9afb0e35feaeb603d53b8475707892068fe69449))
* **lib:** add header guard and inline template specialisations ([72033a1](https://gitlab.com/kwinft/disman/commit/72033a1dbc8e80620bfd319ffa026366c0579d25))
* **lib:** respect global output data on generation ([b52046f](https://gitlab.com/kwinft/disman/commit/b52046fd6dfabe89858a996cce42f7a288152a4d))
* **lib:** set cause on config apply ([78aef32](https://gitlab.com/kwinft/disman/commit/78aef32cf6e4d55180a28761a9a8d361ab495fe6))
* **qscreen:** declare argument maybe_unused ([e8142db](https://gitlab.com/kwinft/disman/commit/e8142dbc117b690f6d722565af66330fd02fb648))
* **randr:** format debug output ([ab86cfc](https://gitlab.com/kwinft/disman/commit/ab86cfccb79253d0e5027f1940979cf2a74862a1))
* **randr:** omit setting the output EDID ([20e2e36](https://gitlab.com/kwinft/disman/commit/20e2e36085eac58f26e7ced82d50baa159083127))
* **randr:** read in config initially ([be6b9f5](https://gitlab.com/kwinft/disman/commit/be6b9f55b3d3755f9c0880215dfd0e88eb6ecb57))
* **randr:** remove output on disconnect ([4cc2b89](https://gitlab.com/kwinft/disman/commit/4cc2b89ce09b63023ea3a8f7dc9a3671c416116f))
* **wayland:** guess KWinFT output type from name ([575be48](https://gitlab.com/kwinft/disman/commit/575be487c4f8ec82f458ef0cbf8f864f2a5be52c))
* **wayland:** report config as applicable on pending other config ([6cda074](https://gitlab.com/kwinft/disman/commit/6cda074c60d2f123d546c3262251096073576f1b))
* **wayland:** set tablet mode state on config retrieval ([34a305f](https://gitlab.com/kwinft/disman/commit/34a305f5f795203edc83d08e1d309038f584c121))
* auto-scale to 110 DPI instead of 150 DPI base value ([86b2884](https://gitlab.com/kwinft/disman/commit/86b288469ffd751844012a8a1f0fa668c9bd22b7))
* enforce geometry on replication ([b372276](https://gitlab.com/kwinft/disman/commit/b372276ee1ed1493da7215d7a7be2d67e4dd13e1))
* prefer Wayland backend by WAYLAND_DISPLAY environment variable ([1fab19d](https://gitlab.com/kwinft/disman/commit/1fab19d4d9177d7a2e30efd716bd02c82794e3c5))
* provide config cause ([9717386](https://gitlab.com/kwinft/disman/commit/9717386a9ff85d55505e908e97b88fd8fcd8c950))
* read in mode info map ([8b2c6d9](https://gitlab.com/kwinft/disman/commit/8b2c6d91ed8ea6d344d28345f5da38c49f21b8a6))
* reenable out-of-process backend mode ([cccffe2](https://gitlab.com/kwinft/disman/commit/cccffe23302374d5c042ffd9018cf2a6654ab9a9))
* replace deprecated QTextStream endl with Qt::endl ([5f05f2e](https://gitlab.com/kwinft/disman/commit/5f05f2e0197ed81a36ee7f4e3ea84962eb07f8f0))
* return early when control file not exists ([e9bfd14](https://gitlab.com/kwinft/disman/commit/e9bfd149b392521f6633293ae214856fbb122349))
* set current config in-process ([0a17b02](https://gitlab.com/kwinft/disman/commit/0a17b02d2394ef9087d15d2e8310ff65d610dea3))
* store and load output enabled value ([d07d883](https://gitlab.com/kwinft/disman/commit/d07d883f0678b9fe637d8c939977ea454c02c23a))
* **wayland:** promote no mode ide debug to warning ([0a96dea](https://gitlab.com/kwinft/disman/commit/0a96deaa700833222ca16b8f02410f603317f325))


### Code Style

* **lib:** rename AbstractBackend to Backend ([c53fa3a](https://gitlab.com/kwinft/disman/commit/c53fa3a9a3c049eaccd2c2bdfd5e131cac7a9281))
* **lib:** replace the List suffix of output and mode maps with Map ([614067b](https://gitlab.com/kwinft/disman/commit/614067bce563cbd7bc71debccde0543e6c413e26))
* **lib:** use snake case for function names ([ccf9660](https://gitlab.com/kwinft/disman/commit/ccf9660f274753f99551400b6cb018e591337eea))


### Build

* move Edid class to backend object library ([d382f63](https://gitlab.com/kwinft/disman/commit/d382f631e4055aad4f7a8e4e9f730fc63a079812))
* rename disman-doctor target to dismanctl ([77da5ee](https://gitlab.com/kwinft/disman/commit/77da5ee302477206fb7747ee1bc54e5a4a38dd57))
* **lib:** namespace target ([eaac664](https://gitlab.com/kwinft/disman/commit/eaac66400f4d0b1a29dd82889c8535de1772aff2))
* **wayland:** namespace library target ([6101ad0](https://gitlab.com/kwinft/disman/commit/6101ad0d33e2378627e088434cb16944812388e6))
* do not generate additional headers ([cfad721](https://gitlab.com/kwinft/disman/commit/cfad721a755c6bef2f2626ba75714195c6ad0cf9))


### Refactors

* abstract config creation ([e02b68c](https://gitlab.com/kwinft/disman/commit/e02b68c89599336c8ddc780a2d0fadb1d41d133e))
* add backend implementation class ([2161f10](https://gitlab.com/kwinft/disman/commit/2161f1084076a74400b31eb41e2d30beacbdc075))
* add separate config implementation ([4bac2f2](https://gitlab.com/kwinft/disman/commit/4bac2f273f803692bab9b2aba6ac590fc03c4c10))
* add separate set config implementation ([4083049](https://gitlab.com/kwinft/disman/commit/40830497c7cfdd3630883dc5800895cf8757ed71))
* handle config change in parent class ([9c01dc2](https://gitlab.com/kwinft/disman/commit/9c01dc204e994979ad01650f40b3d9818e53af74))
* improve log and inline comments ([fb991d9](https://gitlab.com/kwinft/disman/commit/fb991d9f5f503a71420df3dae8c94034807d5501))
* move filer controller to backend implementation ([a8c23b6](https://gitlab.com/kwinft/disman/commit/a8c23b6b8d0f8f290dd8328c5e5c7f9bfd4bb73d))
* move filer write to backend impl class ([3011ab7](https://gitlab.com/kwinft/disman/commit/3011ab791f4d288f44c2a936217f313450e9c8fc))
* move static filer functions to output filer ([4c8982c](https://gitlab.com/kwinft/disman/commit/4c8982c95d6c3d2a3e4b4d443517790729fba62b))
* remove duplicate output logic ([8675b63](https://gitlab.com/kwinft/disman/commit/8675b639934eb5ab962511e0560cef9a462aa971))
* remove output EDID transmission ([af742ad](https://gitlab.com/kwinft/disman/commit/af742adfb3f2f40d980fd1183c4a73418f07e216))
* report back if config read succeeded ([d6837bc](https://gitlab.com/kwinft/disman/commit/d6837bcb440930d687959f54bd370ca7ea19d395))
* take down parsing of Qt properties in fake backend ([5c37696](https://gitlab.com/kwinft/disman/commit/5c3769699d0059a135f0192460e357815d3955fc))
* **api:** remove connected outputs getter ([03cf7e6](https://gitlab.com/kwinft/disman/commit/03cf7e63c71a2055071e170dfb607672e3c68fba))
* **api:** remove output connected information ([2eced9f](https://gitlab.com/kwinft/disman/commit/2eced9ffaf7743259add6f262a52995e76e7ac8c))
* **ctl:** provide parser in Doctor ctor ([b0142f2](https://gitlab.com/kwinft/disman/commit/b0142f2c8c8e9e3d27705469f2b204d7454aaff4))
* **ctl:** remove unused Doctor functions ([391fbc7](https://gitlab.com/kwinft/disman/commit/391fbc79030beeeca3db505fa0be36e44b95e818))
* **ctl:** replace Q_FOREACH with range-based loops ([f7f38f9](https://gitlab.com/kwinft/disman/commit/f7f38f911030483227f53f12b5df11dfb5233663))
* **lib:** improve small Output code bits ([0201b1f](https://gitlab.com/kwinft/disman/commit/0201b1f2031db44b593f327aa2ba3a728d650a5e))
* **lib:** prefer using alias over typedef ([f02ebc5](https://gitlab.com/kwinft/disman/commit/f02ebc54123d06ba9054da2177b4bc260d84a8b4))
* **lib:** remove Edid class inheritance from QObject ([fe00495](https://gitlab.com/kwinft/disman/commit/fe0049523de5d064e4f5f229c6609b4b90bcce9c))
* **lib:** remove Edid Q_PROPERTY macros ([cd6ca98](https://gitlab.com/kwinft/disman/commit/cd6ca984a9e273e698a519a13f7e691d44063090))
* **lib:** remove QObject inheritance from Mode ([bcff7aa](https://gitlab.com/kwinft/disman/commit/bcff7aa94c4bff4e8b3f293da8901b67b0e38f6e))
* **lib:** remove remaining Q_PROPERTY macros ([ea5a79d](https://gitlab.com/kwinft/disman/commit/ea5a79d4ab19ae2d8e0d629f240acd3e95d52761))
* **lib:** replace QSharedPointer with std::shared_ptr ([4e12cb6](https://gitlab.com/kwinft/disman/commit/4e12cb6c5729930193aacb1ad7375ad2c3122f4f))
* remove RandR 1.1 backend ([5f08fbf](https://gitlab.com/kwinft/disman/commit/5f08fbf80294ffcb5a0a96693c2b932219155a39))
* transform backend QMaps to std::map ([f611943](https://gitlab.com/kwinft/disman/commit/f61194325fedb0eac337f1e0eb18b7290ed426ce))
* **lib:** remove unused apply signal code ([21d6dfb](https://gitlab.com/kwinft/disman/commit/21d6dfb2440e6b08706039101a1b7fa92bc7f30b))
* **lib:** transform OutputList to std::map ([4825151](https://gitlab.com/kwinft/disman/commit/4825151ff72c12d112fadd389d7c60c867a70c62))
* use std::string and containers for modes ([f0b8894](https://gitlab.com/kwinft/disman/commit/f0b8894e0d3ca24d8c3454661054236dde26b7a2))
* **ctl:** clean up includes ([ba3331e](https://gitlab.com/kwinft/disman/commit/ba3331ec8952cf21875b6f4b63602f30b471caf1))
* **ctl:** rename logging category ([1d5bea4](https://gitlab.com/kwinft/disman/commit/1d5bea466a1b8b420794d3269cb9a2a5b9afe28c))
* **lib:** access primary Output only by Config ([4eb5178](https://gitlab.com/kwinft/disman/commit/4eb51787f0cffe6f2665b0668189d9de783ccb71))
* **lib:** parse EDID data in private constructor ([acd37c4](https://gitlab.com/kwinft/disman/commit/acd37c4c70db17fb5c3da709e256d1580dfec1ef))
* **lib:** remove default Edid constructor ([13d6af9](https://gitlab.com/kwinft/disman/commit/13d6af9ba3876f0eb6df49d5e16c1e2f730a5909))
* **lib:** remove EDID device-id fallback ([dc2b238](https://gitlab.com/kwinft/disman/commit/dc2b238a394fcbf7bc40bd0732a0714f760a48e4))
* **lib:** replace qreal with double ([e2c2086](https://gitlab.com/kwinft/disman/commit/e2c2086ebeb71a35667b16fb597207114f5700a9))
* **lib:** use double type for mode refresh rate ([f481cc5](https://gitlab.com/kwinft/disman/commit/f481cc519264184d94f13efd3c19458f292c8785))
* **lib:** use std::string in Edid class ([24cc945](https://gitlab.com/kwinft/disman/commit/24cc94583b071d7cc7a8097a13e3117cc99a5adc))
* **qscreen:** create Config to read in backend ([759ad5b](https://gitlab.com/kwinft/disman/commit/759ad5b18aa71f7af25f902427ddcf32d9edc7a5))
* **randr:** create Config to read in backend ([cf790dd](https://gitlab.com/kwinft/disman/commit/cf790dd5a8da9ee185ee5a5f1e7eb9cc00c29c1e))
* **wayland:** adapt to Wrapland output basic information changes ([ab68b35](https://gitlab.com/kwinft/disman/commit/ab68b355d3aa39d8597eda32b329991598cdb5a3))
* remove unneeded includes ([2efdadf](https://gitlab.com/kwinft/disman/commit/2efdadff5039e5bf880b1eed057ee0702edba1d7))
* **lib:** use smart pointer for private class ([79fb32e](https://gitlab.com/kwinft/disman/commit/79fb32ee434f40e3f1e6dab766c713c75235d874))
* **lib:** use std::string for Output name ([bbfa661](https://gitlab.com/kwinft/disman/commit/bbfa661e5fdb3b7d36da596a46aece81e69be3ec))
* **randr:** remove undefined function declaration ([40cfc62](https://gitlab.com/kwinft/disman/commit/40cfc62bd1828ccd5c5ac17d13fdf3e5b055a0de))
* **randr:** set EDID on provided outputs ([eeed75c](https://gitlab.com/kwinft/disman/commit/eeed75c62c63e946eb83d0bd7d20c84414874f1e))
* **wayland:** adapt to Wrapland's changed output device API ([0fe1996](https://gitlab.com/kwinft/disman/commit/0fe1996576d24be8af475931656048f427bea8f1))
* add best resolution, rate and mode getters ([93d5b54](https://gitlab.com/kwinft/disman/commit/93d5b54fb7ee6fd0f6ad2e91f7ea0e7fdce78723))
* add private header file for Output class ([8e1778d](https://gitlab.com/kwinft/disman/commit/8e1778d82d29db3ae5c30accdaaa69199fefcfec))
* improve preferred backend function ([91457f1](https://gitlab.com/kwinft/disman/commit/91457f194a0d5157c73361678faa36830dae51bb))
* provide setter/getter functions for filer write/read ([ca3c756](https://gitlab.com/kwinft/disman/commit/ca3c756b613a5f3def4a73ffdb4b1ae7f0245e5f))
* simplify backend name comparision ([a43ca3b](https://gitlab.com/kwinft/disman/commit/a43ca3beaef65c87989cd5d47c6fcda980b6c52f))
* **randr:** remove unused signal ([15e25e7](https://gitlab.com/kwinft/disman/commit/15e25e7faa9c9e9e86ea34d2f1cc3a307e1f71ff))
* **randr:** return config sent state ([34a56b7](https://gitlab.com/kwinft/disman/commit/34a56b7e2256a799d3180e2ba4d4bcea2ae11d42))
* **wayland:** merge backend and config ([2ed91f6](https://gitlab.com/kwinft/disman/commit/2ed91f623b70d492332f89e4f372f86db4dd8148))
* **wayland:** return config sent state ([1adb9e3](https://gitlab.com/kwinft/disman/commit/1adb9e39a666f79939658af021ab598bbad3b8d8))

## [0.519.0](https://gitlab.com/kwinft/disman/compare/disman@0.519.0-beta.1...disman@0.519.0) (2020-06-09)

## [0.519.0-beta.1](https://gitlab.com/kwinft/disman/compare/disman@0.519.0-beta.0...disman@0.519.0-beta.1) (2020-05-26)


### Build

* set project version of DismanWayland ([f7d5453](https://gitlab.com/kwinft/disman/commit/f7d545376c10b1b7b4046db5aa5d4ad278f0ebbf))

## [0.519.0-beta.0](https://gitlab.com/kwinft/disman/compare/disman@0.0.0...disman@0.519.0-beta.0) (2020-05-24)


### ⚠ BREAKING CHANGES

* The Output API changes.

### Features

* **wayland:** add KWinFT and wlroots plugins ([fa74895](https://gitlab.com/kwinft/disman/commit/fa74895594c8e0e61606c29572be2160cad8c5e5))
* **wayland:** support multiple protocol extensions through plugin system ([91c29f8](https://gitlab.com/kwinft/disman/commit/91c29f8ade1f4fbff0c039f20977e2529becfda2))


### Bug Fixes

* relay logical size correctly ([18ce57f](https://gitlab.com/kwinft/disman/commit/18ce57f372574984e0b907bb294595cd0df1bd2e))
* **wayland:** clean up selected interface thread ([c255bfa](https://gitlab.com/kwinft/disman/commit/c255bfa22d0460a5220a2716847b0439a64e13c5))


### Refactors

* rename project ([0c99006](https://gitlab.com/kwinft/disman/commit/0c99006207815b68fac19d0ac500a13944907281))
* set output geometry ([9215eed](https://gitlab.com/kwinft/disman/commit/9215eede3b89078bc6360da689a7b46f2b01b24a))
