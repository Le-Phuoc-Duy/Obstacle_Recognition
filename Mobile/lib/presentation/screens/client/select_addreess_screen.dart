// import 'package:flutter/material.dart';
// import 'package:flutter_bloc/flutter_bloc.dart';
// import 'package:flutter_svg/flutter_svg.dart';
// import 'package:vision_aid/domain/bloc/blocs.dart';
// import 'package:vision_aid/domain/models/response/addresses_response.dart';
// import 'package:vision_aid/domain/services/services.dart';
// import 'package:vision_aid/presentation/components/components.dart';
// import 'package:vision_aid/presentation/themes/colors_frave.dart';

// class SelectAddressScreen extends StatelessWidget {

//   @override
//   Widget build(BuildContext context){

//     return Scaffold(
//       backgroundColor: Colors.white,
//       appBar: AppBar(
//         title: const TextCustom(text: 'Select Addresses',),
//         backgroundColor: Colors.white,
//         elevation: 0,
//         centerTitle: true,
//         leadingWidth: 80,
//         leading: IconButton(
//           icon: Row(
//             children: const [
//               Icon(Icons.arrow_back_ios_new_rounded, color: ColorsEnum.primaryColor, size: 21),
//               TextCustom(text: 'Back', fontSize: 16, color: ColorsEnum.primaryColor )
//             ],
//           ),
//           onPressed: () => Navigator.pop(context),
//         ),
//       ),
//       body: FutureBuilder<List<ListAddress>>(
//           future: userServices.getAddresses(),
//           builder: (context, snapshot)
//             => (!snapshot.hasData)
//               ? const ShimmerFrave()
//               : _ListAddresses(listAddress: snapshot.data!)
//         ),
//     );
//   }
// }

// class _ListAddresses extends StatelessWidget {

//   final List<ListAddress> listAddress;

//   const _ListAddresses({Key? key, required this.listAddress}) : super(key: key);

//   @override
//   Widget build(BuildContext context) {

//     final userBloc = BlocProvider.of<UserBloc>(context);

//     return ( listAddress.length  != 0 )
//     ? ListView.builder(
//         padding: const EdgeInsets.symmetric(horizontal: 20.0, vertical: 10.0),
//         itemCount: listAddress.length,
//         itemBuilder: (_, i)
//           => Container(
//               height: 70,
//               width: MediaQuery.of(context).size.width,
//               margin: const EdgeInsets.only(bottom: 20.0),
//               decoration: BoxDecoration(
//                 color: Colors.grey[50],
//                 borderRadius: BorderRadius.circular(10.0)
//               ),
//               child: ListTile(
//                 leading: BlocBuilder<UserBloc, UserState>(
//                   builder: (_, state)
//                     => ( listAddress[i].isDefault ) ? Icon(Icons.radio_button_checked_rounded, color: ColorsEnum.primaryColor) : Icon(Icons.radio_button_off_rounded)
//                 ),
//                 title: TextCustom(text: listAddress[i].street, fontSize: 20, fontWeight: FontWeight.w500 ),
//                 subtitle: TextCustom(text: listAddress[i].reference, fontSize: 16, color: ColorsEnum.secundaryColor),
//                 onTap: () => userBloc.add(OnSelectAddressButtonEvent( i, listAddress[i].reference)),
//               ),
//             ),
//         )
//     : _WithoutListAddress();
//   }
// }

// class _WithoutListAddress extends StatelessWidget {

//   @override
//   Widget build(BuildContext context) {
//     return SizedBox(
//       width: MediaQuery.of(context).size.width,
//       child: Column(
//         mainAxisAlignment: MainAxisAlignment.center,
//         children: [
//           SvgPicture.asset('Assets/my-location.svg', height: 400 ),
//           const TextCustom(text: 'Without Address', fontSize: 25, fontWeight: FontWeight.w500, color: ColorsEnum.secundaryColor ),
//           const SizedBox(height: 80),
//         ],
//       ),
//     );
//   }
// }

import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:vision_aid/domain/bloc/blocs.dart';
import 'package:vision_aid/domain/models/response/addresses_response.dart';
import 'package:vision_aid/domain/services/services.dart';
import 'package:vision_aid/presentation/components/components.dart';
import 'package:vision_aid/presentation/helpers/helpers.dart';
import 'package:vision_aid/presentation/screens/client/select_street_address_screen.dart';
import 'package:vision_aid/presentation/themes/colors.dart';

class SelectAddressScreen extends StatefulWidget {
  @override
  _SelectAddressScreenState createState() => _SelectAddressScreenState();
}

class _SelectAddressScreenState extends State<SelectAddressScreen>
    with WidgetsBindingObserver {
  @override
  void initState() {
    WidgetsBinding.instance.addObserver(this);
    super.initState();
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    super.dispose();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) async {
    if (state == AppLifecycleState.resumed) {
      if (await Permission.location.isGranted) {
        Navigator.push(context, routeCustom(page: AddStreetAddressScreen()));
      }
    }
  }

  void accessLocation(PermissionStatus status) {
    switch (status) {
      case PermissionStatus.granted:
        Navigator.push(context, routeCustom(page: SelectStreetAddressScreen()));
        break;
      case PermissionStatus.limited:
        break;
      case PermissionStatus.denied:
      case PermissionStatus.restricted:
      case PermissionStatus.permanentlyDenied:
        openAppSettings();
      case PermissionStatus.provisional:
      // TODO: Handle this case.
    }
  }

  @override
  Widget build(BuildContext context) {
    return BlocListener<UserBloc, UserState>(
      listener: (context, state) {
        if (state is LoadingUserState) {
          modalLoading(context);
        } else if (state is SuccessUserState) {
          Navigator.pop(context);
        } else if (state is FailureUserState) {
          Navigator.pop(context);
          errorMessageSnack(context, state.error);
        }
      },
      child: Scaffold(
        backgroundColor: Colors.white,
        appBar: AppBar(
          backgroundColor: Colors.white,
          title: const TextCustom(text: 'Select Addresses', fontSize: 19),
          centerTitle: true,
          elevation: 0,
          leadingWidth: 80,
          leading: IconButton(
            icon: Row(
              children: const [
                Icon(Icons.arrow_back_ios_new_rounded,
                    color: ColorsEnum.primaryColor, size: 21),
                TextCustom(
                    text: 'Back', fontSize: 16, color: ColorsEnum.primaryColor)
              ],
            ),
            onPressed: () => Navigator.pop(context),
          ),
          actions: [
            TextButton(
                onPressed: () async =>
                    accessLocation(await Permission.location.request()),
                child: const TextCustom(
                    text: 'Other',
                    color: ColorsEnum.primaryColor,
                    fontSize: 17)),
          ],
        ),
        body: FutureBuilder<List<ListAddress>>(
            future: userServices.getAddresses(),
            builder: (context, snapshot) => (!snapshot.hasData)
                ? const ShimmerUI()
                : _ListAddresses(listAddress: snapshot.data!)),
      ),
    );
  }
}

class _ListAddresses extends StatefulWidget {
  List<ListAddress> listAddress;

  _ListAddresses({Key? key, required this.listAddress}) : super(key: key);

  @override
  State<_ListAddresses> createState() => _ListAddressesState();
}

class _ListAddressesState extends State<_ListAddresses> {
  @override
  Widget build(BuildContext context) {
    final userBloc = BlocProvider.of<UserBloc>(context);

    return (widget.listAddress.length != 0)
        ? ListView.builder(
            padding:
                const EdgeInsets.symmetric(horizontal: 20.0, vertical: 10.0),
            itemCount: widget.listAddress.length,
            itemBuilder: (_, i) => Dismissible(
                  key: Key((i + 1).toString()),
                  direction: DismissDirection.endToStart,
                  background: Container(),
                  onDismissed: (direction) =>
                      userBloc.add(OnDeleteStreetAddressEvent(i)),
                  secondaryBackground: Container(
                    alignment: Alignment.centerRight,
                    padding: const EdgeInsets.only(right: 20.0),
                    margin: const EdgeInsets.only(bottom: 20.0),
                    decoration: BoxDecoration(
                        color: Colors.red,
                        borderRadius: BorderRadius.only(
                            topRight: Radius.circular(10.0),
                            bottomRight: Radius.circular(10.0))),
                    child: const Icon(Icons.delete_sweep_rounded,
                        color: Colors.white, size: 38),
                  ),
                  child: Container(
                    height: 70,
                    width: MediaQuery.of(context).size.width,
                    margin: const EdgeInsets.only(bottom: 20.0),
                    decoration: BoxDecoration(
                        color: Colors.grey[50],
                        borderRadius: BorderRadius.circular(10.0)),
                    child: ListTile(
                      leading: BlocBuilder<UserBloc, UserState>(
                          builder: (_, state) =>
                              (widget.listAddress[i].isDefault)
                                  ? Icon(Icons.radio_button_checked_rounded,
                                      color: ColorsEnum.primaryColor)
                                  : Icon(Icons.radio_button_off_rounded)),
                      title: TextCustom(
                          text: widget.listAddress[i].reference,
                          fontSize: 20,
                          fontWeight: FontWeight.w500),
                      // subtitle: TextCustom(
                      //     text: widget.listAddress[i].reference,
                      //     fontSize: 16,
                      //     color: ColorsEnum.secundaryColor),
                      trailing: Icon(Icons.swap_horiz_rounded,
                          color: Colors.red[300]),
                      onTap: () async {
                        userBloc.add(OnSelectAddressButtonEvent(
                            i, widget.listAddress[i]));
                        await userServices.setDefaultAddress(i);

                        setState(() {
                          widget.listAddress.forEach((element) {
                            element.setIsDefault = false;
                          });
                          widget.listAddress[i].setIsDefault = true;
                        });
                      },
                    ),
                  ),
                ))
        : _WithoutListAddress();
  }
}

class _WithoutListAddress extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: MediaQuery.of(context).size.width,
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          SvgPicture.asset('Assets/my-location.svg', height: 400),
          const TextCustom(
              text: 'Without Address',
              fontSize: 25,
              fontWeight: FontWeight.w500,
              color: ColorsEnum.secundaryColor),
          const SizedBox(height: 80),
        ],
      ),
    );
  }
}
